#include <algorithm>
#include <array>
#include <filesystem>
#include <format>
#include <random>
#include <string_view>

#include <boost/asio/connect.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/host_name_verification.hpp>
#include <openssl/ssl.h>

#include "logging/logging.h"
#include "mqtt/mqtt_client.h"
#include "profiling/factories/profiler_factory.h"
#include "profiling/factories/profiling_unit_factory.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Mqtt
{

	namespace
	{
		// MQTT 3.1.1 fixed-header first bytes.  The high nibble is the packet type;
		// the low nibble carries packet-specific flags (SUBSCRIBE must use 0010).
		constexpr uint8_t FixedHeader(PacketType type, uint8_t flags = 0)
		{
			return static_cast<uint8_t>((static_cast<uint8_t>(type) << 4) | (flags & 0x0F));
		}

		// Connect-flags bit positions (MQTT 3.1.1 section 3.1.2.3).
		constexpr uint8_t CONNECT_FLAG_CLEAN_SESSION = 0x02;
		constexpr uint8_t CONNECT_FLAG_WILL = 0x04;
		constexpr uint8_t CONNECT_FLAG_WILL_RETAIN = 0x20;
		constexpr uint8_t CONNECT_FLAG_PASSWORD = 0x40;
		constexpr uint8_t CONNECT_FLAG_USERNAME = 0x80;

		constexpr uint8_t PUBLISH_FLAG_RETAIN = 0x01;

		// Maximum length of an MQTT UTF-8 encoded string (16-bit length prefix).
		constexpr std::size_t MQTT_MAX_STRING_LENGTH = 0xFFFF;

		void EncodeRemainingLength(std::vector<uint8_t>& buf, uint32_t length)
		{
			do
			{
				uint8_t encoded_byte = length % 128;
				length /= 128;
				if (length > 0)
				{
					encoded_byte |= 0x80;
				}
				buf.push_back(encoded_byte);
			} while (length > 0);
		}

		// Append a UTF-8 string with a 16-bit big-endian length prefix.
		// Returns false (and appends nothing) if the string exceeds the field width;
		// the caller treats this as a fatal encoding error rather than silently
		// narrowing the length to a corrupt value.
		[[nodiscard]] bool EncodeUtf8String(std::vector<uint8_t>& buf, std::string_view str)
		{
			if (str.size() > MQTT_MAX_STRING_LENGTH)
			{
				return false;
			}

			const auto len = static_cast<uint16_t>(str.size());
			buf.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
			buf.push_back(static_cast<uint8_t>(len & 0xFF));
			buf.insert(buf.end(), str.begin(), str.end());
			return true;
		}
	}

	MqttClient::MqttClient(boost::asio::io_context& io_context, const Options::Mqtt::MqttSettings& settings)
		: m_IoContext(io_context)
		, m_Settings(settings)
		, m_Resolver(io_context)
	{
		m_ClientId = settings.client_id.empty() ? GenerateClientId() : settings.client_id;
		LogDebug(Channel::Mqtt, [&] { return std::format("MQTT client created with client ID: {}", m_ClientId); });

		// Document the credentials-over-TLS expectation: a plaintext broker
		// connection sends the password as cleartext on the wire.
		if (!m_Settings.password.empty() && !m_Settings.use_tls)
		{
			LogWarning(Channel::Mqtt, "MQTT credentials configured without TLS - password will be sent in cleartext; enable --mqtt-tls");
		}

		if (settings.use_tls)
		{
			InitializeSslContext();
		}
	}

	void MqttClient::InitializeSslContext()
	{
		LogInfo(Channel::Mqtt, "Initializing TLS context for MQTT");

		m_SslContext.emplace(boost::asio::ssl::context::tlsv12_client);

		// Disable old protocols
		m_SslContext->set_options(
			boost::asio::ssl::context::default_workarounds |
			boost::asio::ssl::context::no_sslv2 |
			boost::asio::ssl::context::no_sslv3 |
			boost::asio::ssl::context::no_tlsv1 |
			boost::asio::ssl::context::no_tlsv1_1
		);

		boost::system::error_code ec;

		// Load CA certificate for server verification
		if (!m_Settings.tls_ca_cert.empty())
		{
			if (std::filesystem::exists(m_Settings.tls_ca_cert))
			{
				m_SslContext->load_verify_file(m_Settings.tls_ca_cert, ec);
				if (ec)
				{
					LogError(Channel::Mqtt, [msg = ec.message()] { return std::format("Failed to load CA certificate: {}", msg); });
				}
				else
				{
					LogDebug(Channel::Mqtt, [&] { return std::format("Loaded CA certificate: {}", m_Settings.tls_ca_cert); });
				}
			}
			else
			{
				LogError(Channel::Mqtt, [&] { return std::format("CA certificate file not found: {}", m_Settings.tls_ca_cert); });
			}
		}

		// Load client certificate for mutual TLS
		if (!m_Settings.tls_client_cert.empty() && !m_Settings.tls_client_key.empty())
		{
			if (std::filesystem::exists(m_Settings.tls_client_cert) && std::filesystem::exists(m_Settings.tls_client_key))
			{
				m_SslContext->use_certificate_file(m_Settings.tls_client_cert, boost::asio::ssl::context::pem, ec);
				if (ec)
				{
					LogError(Channel::Mqtt, [msg = ec.message()] { return std::format("Failed to load client certificate: {}", msg); });
				}
				else
				{
					m_SslContext->use_private_key_file(m_Settings.tls_client_key, boost::asio::ssl::context::pem, ec);
					if (ec)
					{
						LogError(Channel::Mqtt, [msg = ec.message()] { return std::format("Failed to load client private key: {}", msg); });
					}
					else
					{
						LogDebug(Channel::Mqtt, "Loaded client certificate and key for mutual TLS");
					}
				}
			}
			else
			{
				LogError(Channel::Mqtt, "Client certificate or key file not found");
			}
		}

		// Set verification mode
		if (m_Settings.tls_skip_verify)
		{
			LogWarning(Channel::Mqtt, "TLS certificate verification DISABLED - not recommended for production");
			m_SslContext->set_verify_mode(boost::asio::ssl::verify_none);
		}
		else
		{
			m_SslContext->set_verify_mode(boost::asio::ssl::verify_peer);

			// Trust store: load the system's default CA bundle so verify_peer works
			// without an explicit --mqtt-ca-cert against publicly-trusted brokers.
			m_SslContext->set_default_verify_paths(ec);
			if (ec)
			{
				LogWarning(Channel::Mqtt, [msg = ec.message()] { return std::format("Failed to load system trust store: {}", msg); });
			}

			LogDebug(Channel::Mqtt, "TLS certificate verification enabled (peer + hostname)");
		}
	}

	MqttClient::~MqttClient()
	{
		Stop();
	}

	void MqttClient::SetWill(const std::string& topic, const std::string& payload, bool retain)
	{
		m_WillConfig = WillConfig{ topic, payload, retain };
		LogDebug(Channel::Mqtt, [&] { return std::format("LWT configured: topic='{}', retain={}", topic, retain); });
	}

	void MqttClient::Start()
	{
		if (m_Running)
		{
			LogWarning(Channel::Mqtt, "MQTT client already running");
			return;
		}

		m_Running = true;
		m_ReconnectAttempts = 0;
		m_State = State::Connecting;

		LogInfo(Channel::Mqtt, [&] { return std::format("Starting MQTT client, connecting to {}:{}",
			m_Settings.broker_host, m_Settings.broker_port); });
	}

	void MqttClient::Stop() noexcept
	{
		if (!m_Running)
		{
			return;
		}

		LogInfo(Channel::Mqtt, "Stopping MQTT client");
		m_Running = false;

		// Send DISCONNECT if connected
		if (m_State == State::Connected && IsSocketOpen())
		{
			boost::system::error_code ec;
			auto disconnect_pkt = EncodeDisconnect();
			WriteSocket(disconnect_pkt, ec);
		}

		CloseSocket();
		m_PublishQueue.clear();
		m_ReadBuffer.clear();
		m_WriteBuffer.clear();
		m_WriteOffset = 0;
		m_State = State::Disconnected;

		OnDisconnected("Client stopped");
	}

	void MqttClient::Poll()
	{
		if (!m_Running)
		{
			return;
		}

		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MqttClient::Poll", std::source_location::current());

		switch (m_State)
		{
		case State::Disconnected:   break;
		case State::Connecting:     PollConnecting(); break;
		case State::SendingConnect: PollSendingConnect(); break;
		case State::WaitingConnack: PollWaitingConnack(); break;
		case State::Connected:      PollConnected(); break;
		case State::Reconnecting:   PollReconnecting(); break;
		}
	}

	bool MqttClient::IsConnected() const noexcept
	{
		return m_State == State::Connected;
	}

	bool MqttClient::IsRunning() const noexcept
	{
		return m_Running;
	}

	MqttClient::State MqttClient::GetState() const noexcept
	{
		return m_State;
	}

	const std::string& MqttClient::ClientId() const noexcept
	{
		return m_ClientId;
	}

	const std::optional<MqttClient::WillConfig>& MqttClient::GetWill() const noexcept
	{
		return m_WillConfig;
	}

	void MqttClient::Publish(const std::string& topic, const std::string& payload, bool retain)
	{
		// Security: Limit queue size to prevent memory exhaustion
		if (m_PublishQueue.size() >= MAX_PUBLISH_QUEUE_SIZE)
		{
			LogWarning(Channel::Mqtt, "MQTT publish queue full, dropping oldest message");
			m_PublishQueue.pop_front();
			++m_DroppedCount;
		}
		m_PublishQueue.push_back({ topic, payload, retain });
	}

	void MqttClient::Subscribe(const std::string& topic_filter, uint8_t qos)
	{
		if (m_State != State::Connected)
		{
			LogWarning(Channel::Mqtt, [&] { return std::format("Cannot subscribe to '{}': not connected", topic_filter); });
			return;
		}

		auto pkt = EncodeSubscribe(topic_filter, qos);
		if (pkt.empty())
		{
			// EncodeSubscribe already logged the reason (e.g. oversized filter).
			return;
		}

		boost::system::error_code ec;
		WriteSocket(pkt, ec);

		if (ec && ec != boost::asio::error::would_block)
		{
			LogWarning(Channel::Mqtt, [&, msg = ec.message()] { return std::format("Failed to send SUBSCRIBE for '{}': {}", topic_filter, msg); });
		}
		else if (!ec)
		{
			LogDebug(Channel::Mqtt, [&] { return std::format("Sent SUBSCRIBE for '{}' (QoS {})", topic_filter, qos); });
		}
	}

	std::string MqttClient::BuildTopic(const std::string& subtopic) const
	{
		if (m_Settings.topic_prefix.empty())
		{
			return subtopic;
		}
		return std::format("{}/{}", m_Settings.topic_prefix, subtopic);
	}

	const std::string& MqttClient::TopicPrefix() const noexcept
	{
		return m_Settings.topic_prefix;
	}

	//=========================================================================
	// Socket I/O helpers (work with both plain TCP and TLS)
	//=========================================================================

	std::size_t MqttClient::WriteSocket(std::span<const uint8_t> data, boost::system::error_code& ec)
	{
		if (m_SslStream)
		{
			return m_SslStream->write_some(boost::asio::buffer(data.data(), data.size()), ec);
		}
		else if (m_Socket)
		{
			return m_Socket->write_some(boost::asio::buffer(data.data(), data.size()), ec);
		}
		ec = boost::asio::error::not_connected;
		return 0;
	}

	std::size_t MqttClient::ReadSocket(std::span<uint8_t> buffer, boost::system::error_code& ec)
	{
		if (m_SslStream)
		{
			return m_SslStream->read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);
		}
		else if (m_Socket)
		{
			return m_Socket->read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);
		}
		ec = boost::asio::error::not_connected;
		return 0;
	}

	void MqttClient::CloseSocket()
	{
		boost::system::error_code ec;

		// Invalidate any outstanding async-connect handlers so a late completion
		// cannot mutate the state machine after the socket has been torn down.
		++m_ConnectGeneration;
		m_ConnectInProgress = false;
		m_Resolver.cancel();

		// Drop any half-written outbound packet; it belongs to the closed connection.
		m_WriteBuffer.clear();
		m_WriteOffset = 0;

		if (m_SslStream)
		{
			// Attempt graceful SSL shutdown (non-blocking, best effort)
			m_SslStream->shutdown(ec);
			m_SslStream.reset();
		}

		if (m_Socket)
		{
			if (m_Socket->is_open())
			{
				m_Socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
				m_Socket->close(ec);
			}
			m_Socket.reset();
		}
	}

	bool MqttClient::IsSocketOpen() const
	{
		if (m_SslStream)
		{
			return m_SslStream->lowest_layer().is_open();
		}
		return m_Socket && m_Socket->is_open();
	}

	//=========================================================================
	// State machine poll methods
	//=========================================================================

	void MqttClient::ScheduleReconnect(const std::string& reason, bool emit_disconnect)
	{
		LogWarning(Channel::Mqtt, [&] { return std::format("MQTT reconnecting (attempt {}): {}", m_ReconnectAttempts + 1, reason); });

		m_LastError = reason;
		CloseSocket();
		m_ReadBuffer.clear();
		m_State = State::Reconnecting;
		m_ReconnectTime = std::chrono::steady_clock::now() + CalculateReconnectDelay();
		++m_ReconnectAttempts;

		if (emit_disconnect)
		{
			OnDisconnected(reason);
		}
	}

	void MqttClient::PollConnecting()
	{
		// Kick off the asynchronous resolve/connect chain exactly once.  Subsequent
		// Poll() calls while still Connecting simply let the io_context drive the
		// in-flight handlers.
		if (!m_ConnectInProgress)
		{
			Factory::ProfilerFactory::Instance().Get()->Message("MQTT: Connecting to broker", static_cast<uint32_t>(UnitColours::Cyan));
			BeginConnect();
		}
	}

	void MqttClient::BeginConnect()
	{
		// The asynchronous connect chain captures shared_from_this() to keep the
		// client alive across pending handlers.  This requires the client to be
		// owned by a shared_ptr (it always is in production via make_shared).
		if (weak_from_this().expired())
		{
			LogError(Channel::Mqtt, "MqttClient must be owned by a shared_ptr to connect; refusing to start async connect");
			ScheduleReconnect("Client not shared-owned");
			return;
		}

		m_ConnectInProgress = true;
		const auto generation = m_ConnectGeneration;

		auto self = shared_from_this();
		m_Resolver.async_resolve(
			m_Settings.broker_host,
			std::to_string(m_Settings.broker_port),
			[self, this, generation](const boost::system::error_code& ec, const boost::asio::ip::tcp::resolver::results_type& endpoints)
			{
				OnResolveComplete(generation, ec, endpoints);
			});
	}

	void MqttClient::OnResolveComplete(uint64_t generation, const boost::system::error_code& ec, const boost::asio::ip::tcp::resolver::results_type& endpoints)
	{
		// Discard stale handlers (a Stop()/reconnect happened while this was in flight).
		if (generation != m_ConnectGeneration || m_State != State::Connecting)
		{
			return;
		}

		if (ec)
		{
			OnError(std::format("DNS resolve failed: {}", ec.message()));
			ScheduleReconnect(std::format("DNS resolve failed for {}:{}: {}",
				m_Settings.broker_host, m_Settings.broker_port, ec.message()));
			return;
		}

		m_Socket.emplace(m_IoContext);

		auto self = shared_from_this();
		boost::asio::async_connect(*m_Socket, endpoints,
			[self, this, generation](const boost::system::error_code& connect_ec, const boost::asio::ip::tcp::endpoint&)
			{
				OnTcpConnectComplete(generation, connect_ec);
			});
	}

	void MqttClient::OnTcpConnectComplete(uint64_t generation, const boost::system::error_code& ec)
	{
		if (generation != m_ConnectGeneration || m_State != State::Connecting)
		{
			return;
		}

		if (ec)
		{
			OnError(std::format("TCP connect failed: {}", ec.message()));
			ScheduleReconnect(std::format("TCP connect failed to {}:{}: {}",
				m_Settings.broker_host, m_Settings.broker_port, ec.message()));
			return;
		}

		LogDebug(Channel::Mqtt, [&] { return std::format("TCP connected to {}:{}", m_Settings.broker_host, m_Settings.broker_port); });

		if (m_Settings.use_tls && m_SslContext)
		{
			StartTlsHandshake(generation);
		}
		else
		{
			EnterSendingConnect();
		}
	}

	void MqttClient::StartTlsHandshake(uint64_t generation)
	{
		m_SslStream.emplace(*m_Socket, *m_SslContext);

		// Set SNI hostname for server name indication.
		if (!SSL_set_tlsext_host_name(m_SslStream->native_handle(), m_Settings.broker_host.c_str()))
		{
			LogWarning(Channel::Mqtt, "Failed to set SNI hostname");
		}

		// Enable RFC 6125 hostname verification against the broker certificate
		// (unless explicitly disabled).  Peer + trust-store verification is set up
		// on the context; this binds the presented certificate to the host we dialled.
		if (!m_Settings.tls_skip_verify)
		{
			boost::system::error_code verify_ec;
			m_SslStream->set_verify_callback(
				boost::asio::ssl::host_name_verification(m_Settings.broker_host), verify_ec);
			if (verify_ec)
			{
				LogError(Channel::Mqtt, [&, msg = verify_ec.message()] { return std::format("Failed to install hostname verification: {}", msg); });
			}
		}

		auto self = shared_from_this();
		// m_SslStream is engaged here: created in EnterTlsHandshake, and set_verify_callback
		// above already dereferenced it in this same scope. Guaranteed by the connect state machine.
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		m_SslStream->async_handshake(boost::asio::ssl::stream_base::client,
			[self, this, generation](const boost::system::error_code& ec)
			{
				OnTlsHandshakeComplete(generation, ec);
			});
	}

	void MqttClient::OnTlsHandshakeComplete(uint64_t generation, const boost::system::error_code& ec)
	{
		if (generation != m_ConnectGeneration || m_State != State::Connecting)
		{
			return;
		}

		if (ec)
		{
			OnError(std::format("TLS handshake failed: {}", ec.message()));
			ScheduleReconnect(std::format("TLS handshake failed: {}", ec.message()));
			return;
		}

		LogInfo(Channel::Mqtt, "TLS handshake completed successfully");
		EnterSendingConnect();
	}

	void MqttClient::EnterSendingConnect()
	{
		// Switch the socket to non-blocking for the subsequent synchronous,
		// would_block-driven I/O performed in the Connected state machine.
		boost::system::error_code ec;
		// m_Socket is engaged once EnterSendingConnect runs (set during the preceding TCP
		// connect). Guaranteed by the connect state machine.
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		m_Socket->non_blocking(true, ec);
		if (ec)
		{
			ScheduleReconnect(std::format("Failed to set non-blocking: {}", ec.message()));
			return;
		}

		m_ConnectInProgress = false;
		m_WriteBuffer.clear();
		m_WriteOffset = 0;
		m_State = State::SendingConnect;
	}

	void MqttClient::PollSendingConnect()
	{
		if (!IsSocketOpen())
		{
			ScheduleReconnect("Socket not open while sending CONNECT");
			return;
		}

		// Serialise the CONNECT packet once into the pending write buffer.
		if (m_WriteBuffer.empty())
		{
			m_WriteBuffer = EncodeConnect();
			m_WriteOffset = 0;
		}

		boost::system::error_code ec;
		if (!DrainWriteBuffer(ec))
		{
			if (ec == boost::asio::error::would_block)
			{
				return; // Try again next poll
			}

			ScheduleReconnect(std::format("Failed to send CONNECT: {}", ec.message()));
			return;
		}

		LogDebug(Channel::Mqtt, [&] { return std::format("Sent CONNECT packet ({} bytes)", m_WriteBuffer.size()); });
		m_WriteBuffer.clear();
		m_WriteOffset = 0;
		m_ReadBuffer.clear();
		m_LastActivity = std::chrono::steady_clock::now();
		m_State = State::WaitingConnack;
	}

	void MqttClient::PollWaitingConnack()
	{
		if (!IsSocketOpen())
		{
			ScheduleReconnect("Socket not open while waiting for CONNACK");
			return;
		}

		// Check for timeout waiting for CONNACK
		auto now = std::chrono::steady_clock::now();
		if (now - m_LastActivity > CONNACK_TIMEOUT)
		{
			ScheduleReconnect("Timeout waiting for CONNACK");
			return;
		}

		// Try to read CONNACK (4 bytes for MQTT 3.1.1)
		std::array<uint8_t, READ_CHUNK_SIZE> buf{};
		boost::system::error_code ec;
		auto bytes_read = ReadSocket(std::span<uint8_t>(buf.data(), buf.size()), ec);

		if (ec == boost::asio::error::would_block)
		{
			return; // No data yet
		}

		if (ec)
		{
			ScheduleReconnect(std::format("Read error waiting for CONNACK: {}", ec.message()));
			return;
		}

		// Security: Check read buffer size before appending to prevent memory exhaustion
		if (m_ReadBuffer.size() + bytes_read > MAX_READ_BUFFER_SIZE)
		{
			ScheduleReconnect("MQTT read buffer overflow attempt blocked");
			return;
		}

		m_ReadBuffer.insert(m_ReadBuffer.end(), buf.begin(), buf.begin() + static_cast<std::ptrdiff_t>(bytes_read));

		// CONNACK is 4 bytes: type(1) + remaining_length(1) + flags(1) + return_code(1)
		if (m_ReadBuffer.size() >= 4)
		{
			if (ParseConnack(m_ReadBuffer))
			{
				m_State = State::Connected;
				m_ReconnectAttempts = 0;
				m_LastActivity = std::chrono::steady_clock::now();
				m_LastPingSent = m_LastActivity;
				m_ReadBuffer.clear();

				LogInfo(Channel::Mqtt, [&] { return std::format("Connected to MQTT broker at {}:{}{}",
					m_Settings.broker_host, m_Settings.broker_port, m_Settings.use_tls ? " (TLS)" : ""); });
				Factory::ProfilerFactory::Instance().Get()->Message("MQTT: Connected", static_cast<uint32_t>(UnitColours::Green));
				OnConnected();
			}
			else
			{
				ScheduleReconnect("CONNACK rejected by broker");
			}
		}
	}

	void MqttClient::PollConnected()
	{
		if (!IsSocketOpen())
		{
			Factory::ProfilerFactory::Instance().Get()->Message("MQTT: Disconnected - Socket closed", static_cast<uint32_t>(UnitColours::Orange));
			ScheduleReconnect("Socket closed", /*emit_disconnect=*/true);
			return;
		}

		Factory::ProfilerFactory::Instance().Get()->PlotValue("MQTT Publish Queue", static_cast<int64_t>(m_PublishQueue.size()));

		SendPendingPublishes();

		// SendPendingPublishes may have torn the connection down on a write error.
		if (m_State != State::Connected)
		{
			return;
		}

		ReadIncoming();

		if (m_State != State::Connected)
		{
			return;
		}

		SendPingreq();
	}

	void MqttClient::PollReconnecting()
	{
		auto now = std::chrono::steady_clock::now();
		if (now >= m_ReconnectTime)
		{
			Factory::ProfilerFactory::Instance().Get()->Message(
				std::format("MQTT: Reconnecting (attempt {})", m_ReconnectAttempts + 1),
				static_cast<uint32_t>(UnitColours::Yellow));
			LogInfo(Channel::Mqtt, [&] { return std::format("Attempting reconnection (attempt {})", m_ReconnectAttempts + 1); });
			m_State = State::Connecting;
		}
	}

	//=========================================================================
	// Connected-state helpers
	//=========================================================================

	bool MqttClient::DrainWriteBuffer(boost::system::error_code& ec)
	{
		// Write the remaining tail of the active outbound packet.  Returns true once
		// the whole buffer has been flushed; on a partial write the offset advances
		// and we report would_block so the caller retries next poll without losing
		// or re-sending already-transmitted bytes.
		while (m_WriteOffset < m_WriteBuffer.size())
		{
			std::span<const uint8_t> remaining(m_WriteBuffer.data() + m_WriteOffset, m_WriteBuffer.size() - m_WriteOffset);
			auto written = WriteSocket(remaining, ec);

			if (ec == boost::asio::error::would_block)
			{
				m_WriteOffset += written;
				return false;
			}

			if (ec)
			{
				return false;
			}

			m_WriteOffset += written;

			if (written == 0)
			{
				// Defensive: a non-error zero-byte write would otherwise spin.
				ec = boost::asio::error::would_block;
				return false;
			}
		}

		ec = {};
		return true;
	}

	void MqttClient::SendPendingPublishes()
	{
		boost::system::error_code ec;

		while (true)
		{
			// Finish flushing any partially-written packet before serialising the next.
			if (m_WriteBuffer.empty())
			{
				if (m_PublishQueue.empty())
				{
					return;
				}

				const auto& front = m_PublishQueue.front();
				m_WriteBuffer = EncodePublish(front.topic, front.payload, front.retain);
				m_WriteOffset = 0;
			}

			if (!DrainWriteBuffer(ec))
			{
				if (ec == boost::asio::error::would_block)
				{
					return; // Try again next poll; offset retained.
				}

				LogWarning(Channel::Mqtt, [&, msg = ec.message()] { return std::format("Publish write error: {}", msg); });
				ScheduleReconnect(std::format("Write error: {}", ec.message()), /*emit_disconnect=*/true);
				return;
			}

			// Whole packet flushed: clear the write buffer and pop the queued publish.
			m_WriteBuffer.clear();
			m_WriteOffset = 0;
			m_LastActivity = std::chrono::steady_clock::now();
			if (!m_PublishQueue.empty())
			{
				m_PublishQueue.pop_front();
				++m_PublishedCount;
			}
		}
	}

	void MqttClient::ReadIncoming()
	{
		std::array<uint8_t, READ_CHUNK_SIZE> buf{};
		boost::system::error_code ec;
		auto bytes_read = ReadSocket(std::span<uint8_t>(buf.data(), buf.size()), ec);

		if (ec == boost::asio::error::would_block)
		{
			return; // No data available
		}

		if (ec)
		{
			ScheduleReconnect(std::format("Read error: {}", ec.message()), /*emit_disconnect=*/true);
			return;
		}

		if (bytes_read > 0)
		{
			m_LastActivity = std::chrono::steady_clock::now();

			// Security: Check read buffer size before appending.  An overflow here
			// means we have lost framing sync (or are being fed garbage); clearing
			// the buffer mid-packet would desynchronise MQTT framing, so we tear the
			// connection down and reconnect cleanly (mirrors the CONNACK path).
			if (m_ReadBuffer.size() + bytes_read > MAX_READ_BUFFER_SIZE)
			{
				ScheduleReconnect("MQTT read buffer overflow in Connected state", /*emit_disconnect=*/true);
				return;
			}

			m_ReadBuffer.insert(m_ReadBuffer.end(), buf.begin(), buf.begin() + static_cast<std::ptrdiff_t>(bytes_read));

			// Process complete packets from the read buffer.
			while (m_ReadBuffer.size() >= 2)
			{
				uint8_t first_byte = m_ReadBuffer[0];
				auto packet_type = static_cast<PacketType>((first_byte >> 4) & 0x0F);

				// Decode the remaining length (variable-length encoding, up to 4 bytes).
				uint32_t remaining_length = 0;
				uint32_t multiplier = 1;
				std::size_t length_bytes = 0;
				bool length_complete = false;

				for (std::size_t j = 1; j < m_ReadBuffer.size() && j <= 4; ++j)
				{
					remaining_length += (m_ReadBuffer[j] & 0x7F) * multiplier;
					multiplier *= 128;
					++length_bytes;

					if ((m_ReadBuffer[j] & 0x80) == 0)
					{
						length_complete = true;
						break;
					}
				}

				if (!length_complete)
				{
					break; // Need more data to decode the remaining length
				}

				std::size_t total_packet_size = 1 + length_bytes + remaining_length;

				if (m_ReadBuffer.size() < total_packet_size)
				{
					break; // Need more data for the complete packet
				}

				// We have a complete packet. Process it.
				std::size_t payload_offset = 1 + length_bytes;

				switch (packet_type)
				{
				case PacketType::Pingresp:
					LogTrace(Channel::Mqtt, "Received PINGRESP");
					break;

				case PacketType::Suback:
					LogDebug(Channel::Mqtt, "Received SUBACK");
					break;

				case PacketType::Publish:
				{
					// Parse QoS-0 PUBLISH: topic length (2 bytes BE) + topic + payload
					if (remaining_length >= 2)
					{
						std::size_t pos = payload_offset;
						uint16_t topic_len = (static_cast<uint16_t>(m_ReadBuffer[pos]) << 8) | m_ReadBuffer[pos + 1];
						pos += 2;

						if (pos + topic_len <= payload_offset + remaining_length)
						{
							auto pos_signed = static_cast<std::ptrdiff_t>(pos);
							auto topic_len_signed = static_cast<std::ptrdiff_t>(topic_len);

							std::string topic(m_ReadBuffer.begin() + pos_signed, m_ReadBuffer.begin() + pos_signed + topic_len_signed);
							pos += topic_len;

							auto payload_end = static_cast<std::ptrdiff_t>(payload_offset + remaining_length);
							pos_signed = static_cast<std::ptrdiff_t>(pos);

							std::string payload(m_ReadBuffer.begin() + pos_signed, m_ReadBuffer.begin() + payload_end);

							LogDebug(Channel::Mqtt, [&] { return std::format("Received PUBLISH: topic='{}', payload_size={}", topic, payload.size()); });
							OnMessageReceived(topic, payload);
						}
					}
					break;
				}

				default:
					LogTrace(Channel::Mqtt, [&] { return std::format("Received MQTT packet type {} (skipping)", static_cast<int>(packet_type)); });
					break;
				}

				// Remove the processed packet from the buffer.
				m_ReadBuffer.erase(m_ReadBuffer.begin(), m_ReadBuffer.begin() + static_cast<std::ptrdiff_t>(total_packet_size));
			}
		}
	}

	void MqttClient::SendPingreq()
	{
		auto now = std::chrono::steady_clock::now();
		if (now - m_LastPingSent < KEEPALIVE_INTERVAL)
		{
			return;
		}

		auto pkt = EncodePingreq();
		boost::system::error_code ec;
		WriteSocket(pkt, ec);

		if (ec && ec != boost::asio::error::would_block)
		{
			ScheduleReconnect(std::format("Ping write error: {}", ec.message()), /*emit_disconnect=*/true);
			return;
		}

		if (!ec)
		{
			m_LastPingSent = now;
			LogTrace(Channel::Mqtt, "Sent PINGREQ");
		}
	}

	//=========================================================================
	// MQTT 3.1.1 packet encoding
	//=========================================================================

	std::vector<uint8_t> MqttClient::EncodeConnect()
	{
		// Variable header + payload
		std::vector<uint8_t> var_payload;

		// Protocol Name: "MQTT" (4 bytes, well within the length field)
		static_cast<void>(EncodeUtf8String(var_payload, "MQTT"));

		// Protocol Level: 4 (MQTT 3.1.1)
		var_payload.push_back(0x04);

		// Connect Flags (MQTT 3.1.1 section 3.1.2.3)
		uint8_t flags = CONNECT_FLAG_CLEAN_SESSION;

		const bool has_username = !m_Settings.username.empty();
		// MQTT 3.1.1 [MQTT-3.1.2-22]: the Password flag is only valid when the
		// Username flag is also set.  Send the password only alongside a username.
		const bool has_password = has_username && !m_Settings.password.empty();
		const bool has_will = m_WillConfig.has_value();

		if (has_username)
		{
			flags |= CONNECT_FLAG_USERNAME;
		}
		if (has_password)
		{
			flags |= CONNECT_FLAG_PASSWORD;
		}
		if (has_will)
		{
			flags |= CONNECT_FLAG_WILL;
			if (m_WillConfig->retain)
			{
				flags |= CONNECT_FLAG_WILL_RETAIN;
			}
			// Will QoS = 0 (bits 4-3 remain 0)
		}

		if (!m_Settings.password.empty() && !has_username)
		{
			LogWarning(Channel::Mqtt, "MQTT password configured without a username - omitting password from CONNECT (MQTT 3.1.1 requires the username flag)");
		}

		var_payload.push_back(flags);

		// Keep Alive
		var_payload.push_back(static_cast<uint8_t>((KEEPALIVE_SECONDS >> 8) & 0xFF));
		var_payload.push_back(static_cast<uint8_t>(KEEPALIVE_SECONDS & 0xFF));

		// Payload (in order per MQTT 3.1.1 section 3.1.3):
		// Client Identifier, Will Topic, Will Message, Username, Password
		bool encode_ok = EncodeUtf8String(var_payload, m_ClientId);

		if (has_will)
		{
			encode_ok = EncodeUtf8String(var_payload, m_WillConfig->topic) && encode_ok;
			encode_ok = EncodeUtf8String(var_payload, m_WillConfig->payload) && encode_ok;
		}

		if (has_username)
		{
			encode_ok = EncodeUtf8String(var_payload, m_Settings.username) && encode_ok;
		}
		if (has_password)
		{
			encode_ok = EncodeUtf8String(var_payload, m_Settings.password) && encode_ok;
		}

		if (!encode_ok)
		{
			LogError(Channel::Mqtt, "CONNECT field exceeds the 16-bit MQTT string length limit");
		}

		// Build fixed header
		std::vector<uint8_t> packet;
		packet.push_back(FixedHeader(PacketType::Connect));
		EncodeRemainingLength(packet, static_cast<uint32_t>(var_payload.size()));
		packet.insert(packet.end(), var_payload.begin(), var_payload.end());

		// Note: the username/password are deliberately NOT logged (credential leak).
		if (has_username)
		{
			LogDebug(Channel::Mqtt, "CONNECT packet includes username and authentication credentials");
		}

		return packet;
	}

	std::vector<uint8_t> MqttClient::EncodePublish(const std::string& topic, const std::string& payload, bool retain)
	{
		// Variable header: topic name
		std::vector<uint8_t> var_payload;
		if (!EncodeUtf8String(var_payload, topic))
		{
			LogWarning(Channel::Mqtt, [&] { return std::format("PUBLISH topic exceeds the 16-bit length limit ({} bytes), dropping", topic.size()); });
			return {};
		}

		// QoS 0: no packet identifier
		// Payload
		var_payload.insert(var_payload.end(), payload.begin(), payload.end());

		// Fixed header: PUBLISH, DUP=0, QoS=0, RETAIN per flag (MQTT 3.1.1 section 3.3.1.3)
		std::vector<uint8_t> packet;
		packet.push_back(FixedHeader(PacketType::Publish, retain ? PUBLISH_FLAG_RETAIN : 0));
		EncodeRemainingLength(packet, static_cast<uint32_t>(var_payload.size()));
		packet.insert(packet.end(), var_payload.begin(), var_payload.end());

		return packet;
	}

	std::vector<uint8_t> MqttClient::EncodeSubscribe(const std::string& topic_filter, uint8_t qos)
	{
		// Variable header: packet identifier (2 bytes)
		std::vector<uint8_t> var_payload;
		uint16_t packet_id = m_NextPacketId++;
		if (m_NextPacketId == 0) { m_NextPacketId = 1; } // Packet IDs must be non-zero
		var_payload.push_back(static_cast<uint8_t>((packet_id >> 8) & 0xFF));
		var_payload.push_back(static_cast<uint8_t>(packet_id & 0xFF));

		// Payload: UTF-8 topic filter + requested QoS byte
		if (!EncodeUtf8String(var_payload, topic_filter))
		{
			LogWarning(Channel::Mqtt, [&] { return std::format("SUBSCRIBE filter exceeds the 16-bit length limit ({} bytes), dropping", topic_filter.size()); });
			return {};
		}
		var_payload.push_back(qos & 0x03); // QoS is bits 0-1

		// Fixed header: SUBSCRIBE = 0x82 (type 8, reserved bits = 0010)
		std::vector<uint8_t> packet;
		packet.push_back(FixedHeader(PacketType::Subscribe, 0x02));
		EncodeRemainingLength(packet, static_cast<uint32_t>(var_payload.size()));
		packet.insert(packet.end(), var_payload.begin(), var_payload.end());

		return packet;
	}

	std::vector<uint8_t> MqttClient::EncodePingreq()
	{
		return { FixedHeader(PacketType::Pingreq), 0x00 };
	}

	std::vector<uint8_t> MqttClient::EncodeDisconnect()
	{
		return { FixedHeader(PacketType::Disconnect), 0x00 };
	}

	//=========================================================================
	// MQTT packet parsing
	//=========================================================================

	bool MqttClient::ParseConnack(const std::vector<uint8_t>& data)
	{
		if (data.size() < 4)
		{
			return false;
		}

		// Byte 0: packet type (0x20 = CONNACK)
		if (static_cast<PacketType>((data[0] >> 4) & 0x0F) != PacketType::Connack)
		{
			LogWarning(Channel::Mqtt, [&] { return std::format("Expected CONNACK (0x20), got 0x{:02X}", data[0]); });
			return false;
		}

		// Byte 1: remaining length (should be 2)
		if (data[1] != 0x02)
		{
			LogWarning(Channel::Mqtt, [&] { return std::format("Unexpected CONNACK remaining length: {}", data[1]); });
			return false;
		}

		// Byte 2: connect acknowledge flags (session present)
		// Byte 3: connect return code
		uint8_t return_code = data[3];

		if (return_code != 0x00)
		{
			const char* reason = "Unknown";
			switch (return_code)
			{
			case 0x01: reason = "Unacceptable protocol version"; break;
			case 0x02: reason = "Identifier rejected"; break;
			case 0x03: reason = "Server unavailable"; break;
			case 0x04: reason = "Bad username or password"; break;
			case 0x05: reason = "Not authorized"; break;
			default: break;
			}
			LogWarning(Channel::Mqtt, [&] { return std::format("CONNACK rejected: {} (code {})", reason, return_code); });
			return false;
		}

		return true;
	}

	//=========================================================================
	// Utilities
	//=========================================================================

	std::string MqttClient::GenerateClientId() const
	{
		std::random_device rd;
		std::uniform_int_distribution<> dis(0, 15);

		const char* hex = "0123456789abcdef";
		std::string result = "aqualink-";

		for (int i = 0; i < 8; ++i)
		{
			result += hex[dis(rd)];
		}

		return result;
	}

	std::chrono::seconds MqttClient::CalculateReconnectDelay() const
	{
		using Rep = std::chrono::seconds::rep;
		auto base_delay = m_Settings.reconnect_delay_initial.count();
		auto max_delay = m_Settings.reconnect_delay_max.count();

		auto delay = base_delay * (Rep{1} << std::min(m_ReconnectAttempts, static_cast<uint16_t>(10)));
		delay = std::min(delay, max_delay);

		std::random_device rd;
		std::uniform_int_distribution<> jitter(0, static_cast<int>(delay / 4));
		delay += jitter(rd);

		return std::chrono::seconds(delay);
	}

}
// namespace AqualinkAutomate::Mqtt
