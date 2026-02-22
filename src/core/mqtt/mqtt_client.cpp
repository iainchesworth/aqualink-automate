#include <algorithm>
#include <filesystem>
#include <format>
#include <random>

#include <boost/asio/connect.hpp>
#include <boost/asio/ssl.hpp>
#include <openssl/ssl.h>

#include "logging/logging.h"
#include "mqtt/mqtt_client.h"
#include "profiling/factories/profiler_factory.h"
#include "profiling/factories/profiling_unit_factory.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Mqtt
{

	MqttClient::MqttClient(boost::asio::io_context& io_context, const Options::Mqtt::MqttSettings& settings)
		: m_IoContext(io_context)
		, m_Settings(settings)
	{
		m_ClientId = settings.client_id.empty() ? GenerateClientId() : settings.client_id;
		LogDebug(Channel::Mqtt, std::format("MQTT client created with client ID: {}", m_ClientId));

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
					LogWarning(Channel::Mqtt, std::format("Failed to load CA certificate: {}", ec.message()));
				}
				else
				{
					LogDebug(Channel::Mqtt, std::format("Loaded CA certificate: {}", m_Settings.tls_ca_cert));
				}
			}
			else
			{
				LogWarning(Channel::Mqtt, std::format("CA certificate file not found: {}", m_Settings.tls_ca_cert));
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
					LogWarning(Channel::Mqtt, std::format("Failed to load client certificate: {}", ec.message()));
				}
				else
				{
					m_SslContext->use_private_key_file(m_Settings.tls_client_key, boost::asio::ssl::context::pem, ec);
					if (ec)
					{
						LogWarning(Channel::Mqtt, std::format("Failed to load client private key: {}", ec.message()));
					}
					else
					{
						LogDebug(Channel::Mqtt, "Loaded client certificate and key for mutual TLS");
					}
				}
			}
			else
			{
				LogWarning(Channel::Mqtt, "Client certificate or key file not found");
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
			LogDebug(Channel::Mqtt, "TLS certificate verification enabled");
		}
	}

	MqttClient::~MqttClient()
	{
		Stop();
	}

	void MqttClient::SetWill(const std::string& topic, const std::string& payload, bool retain)
	{
		m_WillConfig = WillConfig{ topic, payload, retain };
		LogDebug(Channel::Mqtt, std::format("LWT configured: topic='{}', retain={}", topic, retain));
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

		LogInfo(Channel::Mqtt, std::format("Starting MQTT client, connecting to {}:{}",
			m_Settings.broker_host, m_Settings.broker_port));
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
		}
		m_PublishQueue.push_back({ topic, payload, retain });
	}

	void MqttClient::Subscribe(const std::string& topic_filter, uint8_t qos)
	{
		if (m_State != State::Connected)
		{
			LogWarning(Channel::Mqtt, std::format("Cannot subscribe to '{}': not connected", topic_filter));
			return;
		}

		auto pkt = EncodeSubscribe(topic_filter, qos);
		boost::system::error_code ec;
		WriteSocket(pkt, ec);

		if (ec && ec != boost::asio::error::would_block)
		{
			LogWarning(Channel::Mqtt, std::format("Failed to send SUBSCRIBE for '{}': {}", topic_filter, ec.message()));
		}
		else if (!ec)
		{
			LogDebug(Channel::Mqtt, std::format("Sent SUBSCRIBE for '{}' (QoS {})", topic_filter, qos));
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

	std::size_t MqttClient::WriteSocket(const std::vector<uint8_t>& data, boost::system::error_code& ec)
	{
		if (m_SslStream)
		{
			return m_SslStream->write_some(boost::asio::buffer(data), ec);
		}
		else if (m_Socket)
		{
			return m_Socket->write_some(boost::asio::buffer(data), ec);
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

	void MqttClient::PollConnecting()
	{
		Factory::ProfilerFactory::Instance().Get()->Message("MQTT: Connecting to broker", static_cast<uint32_t>(UnitColours::Cyan));

		// Create a non-blocking TCP socket and attempt synchronous connect
		boost::system::error_code ec;

		boost::asio::ip::tcp::resolver resolver(m_IoContext);
		auto endpoints = resolver.resolve(
			m_Settings.broker_host,
			std::to_string(m_Settings.broker_port),
			ec);

		if (ec)
		{
			LogWarning(Channel::Mqtt, std::format("DNS resolve failed for {}:{}: {}",
				m_Settings.broker_host, m_Settings.broker_port, ec.message()));
			OnError(std::format("DNS resolve failed: {}", ec.message()));
			m_State = State::Reconnecting;
			m_ReconnectTime = std::chrono::steady_clock::now() + CalculateReconnectDelay();
			++m_ReconnectAttempts;
			return;
		}

		m_Socket.emplace(m_IoContext);
		boost::asio::connect(*m_Socket, endpoints, ec);

		if (ec)
		{
			LogWarning(Channel::Mqtt, std::format("TCP connect failed to {}:{}: {}",
				m_Settings.broker_host, m_Settings.broker_port, ec.message()));
			OnError(std::format("TCP connect failed: {}", ec.message()));
			CloseSocket();
			m_State = State::Reconnecting;
			m_ReconnectTime = std::chrono::steady_clock::now() + CalculateReconnectDelay();
			++m_ReconnectAttempts;
			return;
		}

		LogDebug(Channel::Mqtt, std::format("TCP connected to {}:{}", m_Settings.broker_host, m_Settings.broker_port));

		// Perform TLS handshake if enabled
		if (m_Settings.use_tls && m_SslContext)
		{
			m_SslStream.emplace(*m_Socket, *m_SslContext);

			// Set SNI hostname for server name indication
			if (!SSL_set_tlsext_host_name(m_SslStream->native_handle(), m_Settings.broker_host.c_str()))
			{
				LogWarning(Channel::Mqtt, "Failed to set SNI hostname");
			}

			m_SslStream->handshake(boost::asio::ssl::stream_base::client, ec);

			if (ec)
			{
				LogWarning(Channel::Mqtt, std::format("TLS handshake failed: {}", ec.message()));
				OnError(std::format("TLS handshake failed: {}", ec.message()));
				CloseSocket();
				m_State = State::Reconnecting;
				m_ReconnectTime = std::chrono::steady_clock::now() + CalculateReconnectDelay();
				++m_ReconnectAttempts;
				return;
			}

			LogInfo(Channel::Mqtt, "TLS handshake completed successfully");
		}

		// Set socket to non-blocking for subsequent I/O
		m_Socket->non_blocking(true, ec);
		if (ec)
		{
			LogWarning(Channel::Mqtt, std::format("Failed to set non-blocking: {}", ec.message()));
			CloseSocket();
			m_State = State::Reconnecting;
			m_ReconnectTime = std::chrono::steady_clock::now() + CalculateReconnectDelay();
			++m_ReconnectAttempts;
			return;
		}

		m_State = State::SendingConnect;
	}

	void MqttClient::PollSendingConnect()
	{
		if (!IsSocketOpen())
		{
			m_State = State::Reconnecting;
			m_ReconnectTime = std::chrono::steady_clock::now() + CalculateReconnectDelay();
			++m_ReconnectAttempts;
			return;
		}

		auto connect_pkt = EncodeConnect();
		boost::system::error_code ec;
		auto bytes_written = WriteSocket(connect_pkt, ec);

		if (ec == boost::asio::error::would_block)
		{
			return; // Try again next poll
		}

		if (ec)
		{
			LogWarning(Channel::Mqtt, std::format("Failed to send CONNECT: {}", ec.message()));
			CloseSocket();
			m_State = State::Reconnecting;
			m_ReconnectTime = std::chrono::steady_clock::now() + CalculateReconnectDelay();
			++m_ReconnectAttempts;
			return;
		}

		LogDebug(Channel::Mqtt, std::format("Sent CONNECT packet ({} bytes)", bytes_written));
		m_ReadBuffer.clear();
		m_LastActivity = std::chrono::steady_clock::now();
		m_State = State::WaitingConnack;
	}

	void MqttClient::PollWaitingConnack()
	{
		if (!IsSocketOpen())
		{
			m_State = State::Reconnecting;
			m_ReconnectTime = std::chrono::steady_clock::now() + CalculateReconnectDelay();
			++m_ReconnectAttempts;
			return;
		}

		// Check for timeout waiting for CONNACK
		auto now = std::chrono::steady_clock::now();
		if (now - m_LastActivity > std::chrono::seconds(10))
		{
			LogWarning(Channel::Mqtt, "Timeout waiting for CONNACK");
			CloseSocket();
			m_State = State::Reconnecting;
			m_ReconnectTime = now + CalculateReconnectDelay();
			++m_ReconnectAttempts;
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
			LogWarning(Channel::Mqtt, std::format("Read error waiting for CONNACK: {}", ec.message()));
			CloseSocket();
			m_State = State::Reconnecting;
			m_ReconnectTime = std::chrono::steady_clock::now() + CalculateReconnectDelay();
			++m_ReconnectAttempts;
			return;
		}

		// Security: Check read buffer size before appending to prevent memory exhaustion
		if (m_ReadBuffer.size() + bytes_read > MAX_READ_BUFFER_SIZE)
		{
			LogWarning(Channel::Mqtt, "MQTT read buffer overflow attempt blocked, disconnecting");
			CloseSocket();
			m_ReadBuffer.clear();
			m_State = State::Reconnecting;
			m_ReconnectTime = std::chrono::steady_clock::now() + CalculateReconnectDelay();
			++m_ReconnectAttempts;
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

				LogInfo(Channel::Mqtt, std::format("Connected to MQTT broker at {}:{}{}",
					m_Settings.broker_host, m_Settings.broker_port, m_Settings.use_tls ? " (TLS)" : ""));
				Factory::ProfilerFactory::Instance().Get()->Message("MQTT: Connected", static_cast<uint32_t>(UnitColours::Green));
				OnConnected();
			}
			else
			{
				LogWarning(Channel::Mqtt, "CONNACK rejected by broker");
				CloseSocket();
				m_ReadBuffer.clear();
				m_State = State::Reconnecting;
				m_ReconnectTime = std::chrono::steady_clock::now() + CalculateReconnectDelay();
				++m_ReconnectAttempts;
			}
		}
	}

	void MqttClient::PollConnected()
	{
		if (!IsSocketOpen())
		{
			LogWarning(Channel::Mqtt, "Socket closed unexpectedly");
			Factory::ProfilerFactory::Instance().Get()->Message("MQTT: Disconnected - Socket closed", static_cast<uint32_t>(UnitColours::Orange));
			m_State = State::Reconnecting;
			m_ReconnectTime = std::chrono::steady_clock::now() + CalculateReconnectDelay();
			++m_ReconnectAttempts;
			OnDisconnected("Socket closed");
			return;
		}

		Factory::ProfilerFactory::Instance().Get()->PlotValue("MQTT Publish Queue", static_cast<int64_t>(m_PublishQueue.size()));

		SendPendingPublishes();
		ReadIncoming();
		SendPingreq();
	}

	void MqttClient::PollReconnecting()
	{
		auto now = std::chrono::steady_clock::now();
		if (now >= m_ReconnectTime)
		{
			auto msg = std::format("MQTT: Reconnecting (attempt {})", m_ReconnectAttempts + 1);
			Factory::ProfilerFactory::Instance().Get()->Message(msg, static_cast<uint32_t>(UnitColours::Yellow));
			LogDebug(Channel::Mqtt, std::format("Attempting reconnection (attempt {})", m_ReconnectAttempts + 1));
			m_State = State::Connecting;
		}
	}

	//=========================================================================
	// Connected-state helpers
	//=========================================================================

	void MqttClient::SendPendingPublishes()
	{
		while (!m_PublishQueue.empty())
		{
			auto& front = m_PublishQueue.front();
			auto pkt = EncodePublish(front.topic, front.payload, front.retain);

			boost::system::error_code ec;
			WriteSocket(pkt, ec);

			if (ec == boost::asio::error::would_block)
			{
				break; // Try again next poll
			}

			if (ec)
			{
				LogDebug(Channel::Mqtt, std::format("Publish write error: {}", ec.message()));
				CloseSocket();
				m_State = State::Reconnecting;
				m_ReconnectTime = std::chrono::steady_clock::now() + CalculateReconnectDelay();
				++m_ReconnectAttempts;
				OnDisconnected(std::format("Write error: {}", ec.message()));
				return;
			}

			m_LastActivity = std::chrono::steady_clock::now();
			m_PublishQueue.pop_front();
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
			LogDebug(Channel::Mqtt, std::format("Read error: {}", ec.message()));
			CloseSocket();
			m_State = State::Reconnecting;
			m_ReconnectTime = std::chrono::steady_clock::now() + CalculateReconnectDelay();
			++m_ReconnectAttempts;
			OnDisconnected(std::format("Read error: {}", ec.message()));
			return;
		}

		if (bytes_read > 0)
		{
			m_LastActivity = std::chrono::steady_clock::now();

			// Security: Check read buffer size before appending
			if (m_ReadBuffer.size() + bytes_read > MAX_READ_BUFFER_SIZE)
			{
				LogWarning(Channel::Mqtt, "MQTT read buffer overflow in Connected state, clearing buffer");
				m_ReadBuffer.clear();
				return;
			}

			m_ReadBuffer.insert(m_ReadBuffer.end(), buf.begin(), buf.begin() + static_cast<std::ptrdiff_t>(bytes_read));

			// Process complete packets from the read buffer.
			while (m_ReadBuffer.size() >= 2)
			{
				uint8_t first_byte = m_ReadBuffer[0];
				uint8_t packet_type = (first_byte >> 4) & 0x0F;

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

				if (packet_type == 13) // PINGRESP
				{
					LogTrace(Channel::Mqtt, "Received PINGRESP");
				}
				else if (packet_type == 9) // SUBACK
				{
					LogDebug(Channel::Mqtt, "Received SUBACK");
				}
				else if (packet_type == 3) // PUBLISH
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

							LogDebug(Channel::Mqtt, std::format("Received PUBLISH: topic='{}', payload_size={}", topic, payload.size()));
							OnMessageReceived(topic, payload);
						}
					}
				}
				else
				{
					LogTrace(Channel::Mqtt, std::format("Received MQTT packet type {} (skipping)", packet_type));
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
			LogDebug(Channel::Mqtt, std::format("Ping write error: {}", ec.message()));
			CloseSocket();
			m_State = State::Reconnecting;
			m_ReconnectTime = now + CalculateReconnectDelay();
			++m_ReconnectAttempts;
			OnDisconnected(std::format("Ping write error: {}", ec.message()));
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

	static void EncodeRemainingLength(std::vector<uint8_t>& buf, uint32_t length)
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

	static void EncodeUtf8String(std::vector<uint8_t>& buf, const std::string& str)
	{
		auto len = static_cast<uint16_t>(str.size());
		buf.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
		buf.push_back(static_cast<uint8_t>(len & 0xFF));
		buf.insert(buf.end(), str.begin(), str.end());
	}

	std::vector<uint8_t> MqttClient::EncodeConnect()
	{
		// Variable header + payload
		std::vector<uint8_t> var_payload;

		// Protocol Name: "MQTT"
		EncodeUtf8String(var_payload, "MQTT");

		// Protocol Level: 4 (MQTT 3.1.1)
		var_payload.push_back(0x04);

		// Connect Flags
		// Bit 7: Username Flag
		// Bit 6: Password Flag
		// Bit 5: Will Retain
		// Bit 4-3: Will QoS
		// Bit 2: Will Flag
		// Bit 1: Clean Session
		// Bit 0: Reserved
		uint8_t flags = 0x02; // Clean Session

		bool has_username = !m_Settings.username.empty();
		bool has_password = !m_Settings.password.empty();
		bool has_will = m_WillConfig.has_value();

		if (has_username)
		{
			flags |= 0x80; // Username Flag (bit 7)
		}
		if (has_password)
		{
			flags |= 0x40; // Password Flag (bit 6)
		}
		if (has_will)
		{
			flags |= 0x04; // Will Flag (bit 2)
			if (m_WillConfig->retain)
			{
				flags |= 0x20; // Will Retain (bit 5)
			}
			// Will QoS = 0 (bits 4-3 remain 0)
		}

		var_payload.push_back(flags);

		// Keep Alive
		var_payload.push_back(static_cast<uint8_t>((KEEPALIVE_SECONDS >> 8) & 0xFF));
		var_payload.push_back(static_cast<uint8_t>(KEEPALIVE_SECONDS & 0xFF));

		// Payload (in order per MQTT 3.1.1 section 3.1.3):
		// Client Identifier, Will Topic, Will Message, Username, Password
		EncodeUtf8String(var_payload, m_ClientId);

		if (has_will)
		{
			EncodeUtf8String(var_payload, m_WillConfig->topic);
			EncodeUtf8String(var_payload, m_WillConfig->payload);
		}

		if (has_username)
		{
			EncodeUtf8String(var_payload, m_Settings.username);
		}
		if (has_password)
		{
			EncodeUtf8String(var_payload, m_Settings.password);
		}

		// Build fixed header
		std::vector<uint8_t> packet;
		packet.push_back(0x10); // CONNECT packet type
		EncodeRemainingLength(packet, static_cast<uint32_t>(var_payload.size()));
		packet.insert(packet.end(), var_payload.begin(), var_payload.end());

		if (has_username)
		{
			LogDebug(Channel::Mqtt, std::format("CONNECT packet includes username '{}'", m_Settings.username));
		}

		return packet;
	}

	std::vector<uint8_t> MqttClient::EncodePublish(const std::string& topic, const std::string& payload, bool retain)
	{
		// Variable header: topic name
		std::vector<uint8_t> var_payload;
		EncodeUtf8String(var_payload, topic);

		// QoS 0: no packet identifier
		// Payload
		var_payload.insert(var_payload.end(), payload.begin(), payload.end());

		// Fixed header: PUBLISH, DUP=0, QoS=0, RETAIN per flag (MQTT 3.1.1 section 3.3.1.3)
		std::vector<uint8_t> packet;
		packet.push_back(static_cast<uint8_t>(0x30 | (retain ? 0x01 : 0x00)));
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
		EncodeUtf8String(var_payload, topic_filter);
		var_payload.push_back(qos & 0x03); // QoS is bits 0-1

		// Fixed header: SUBSCRIBE = 0x82 (type 8, reserved bits = 0010)
		std::vector<uint8_t> packet;
		packet.push_back(0x82);
		EncodeRemainingLength(packet, static_cast<uint32_t>(var_payload.size()));
		packet.insert(packet.end(), var_payload.begin(), var_payload.end());

		return packet;
	}

	std::vector<uint8_t> MqttClient::EncodePingreq()
	{
		return { 0xC0, 0x00 };
	}

	std::vector<uint8_t> MqttClient::EncodeDisconnect()
	{
		return { 0xE0, 0x00 };
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
		if ((data[0] & 0xF0) != 0x20)
		{
			LogWarning(Channel::Mqtt, std::format("Expected CONNACK (0x20), got 0x{:02X}", data[0]));
			return false;
		}

		// Byte 1: remaining length (should be 2)
		if (data[1] != 0x02)
		{
			LogWarning(Channel::Mqtt, std::format("Unexpected CONNACK remaining length: {}", data[1]));
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
			LogWarning(Channel::Mqtt, std::format("CONNACK rejected: {} (code {})", reason, return_code));
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
