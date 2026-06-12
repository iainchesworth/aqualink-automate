#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/signals2.hpp>

#include "options/options_mqtt_options.h"

namespace AqualinkAutomate::Test { class MqttClientPacketTest; }

namespace AqualinkAutomate::Mqtt
{

	/// MQTT 3.1.1 control packet types (high nibble of the fixed-header byte).
	///
	/// Shared by both the encode and decode paths so that no magic packet-type
	/// numbers are scattered across the implementation.
	enum class PacketType : uint8_t
	{
		Connect = 1,
		Connack = 2,
		Publish = 3,
		Puback = 4,
		Subscribe = 8,
		Suback = 9,
		Unsubscribe = 10,
		Unsuback = 11,
		Pingreq = 12,
		Pingresp = 13,
		Disconnect = 14
	};

	/// Poll-based MQTT 3.1.1 client with an asynchronous, non-blocking state machine.
	///
	/// Thread-safety contract: This class is NOT thread-safe. All public methods
	/// (including Publish(), Poll(), Start(), Stop()) must be called from the
	/// same thread that runs the associated io_context. The internal publish
	/// queue and connection state are accessed without synchronization,
	/// relying on single-threaded execution within the io_context poll loop.
	///
	/// Connection establishment (DNS resolve, TCP connect, TLS handshake) is fully
	/// non-blocking: each phase is launched with an Asio async operation whose
	/// completion handler advances the state machine. The handlers are driven by
	/// the shared io_context.poll() that the host frame loop already runs, so a
	/// slow or unreachable broker never blocks the ~1ms cooperative frame loop.
	class MqttClient : public std::enable_shared_from_this<MqttClient>
	{
	public:
		using ConnectedSignal = boost::signals2::signal<void()>;
		using DisconnectedSignal = boost::signals2::signal<void(const std::string& reason)>;
		using MessageReceivedSignal = boost::signals2::signal<void(const std::string& topic, const std::string& payload)>;
		using ErrorSignal = boost::signals2::signal<void(const std::string& error_message)>;

		enum class State
		{
			Disconnected,
			Connecting,
			SendingConnect,
			WaitingConnack,
			Connected,
			Reconnecting
		};

	public:
		MqttClient(boost::asio::io_context& io_context, const Options::Mqtt::MqttSettings& settings);
		~MqttClient();

		MqttClient(const MqttClient&) = delete;
		MqttClient& operator=(const MqttClient&) = delete;

	public:
		struct WillConfig
		{
			std::string topic;
			std::string payload;
			bool retain{ true };
		};

		void SetWill(const std::string& topic, const std::string& payload, bool retain = true);

	public:
		void Start();
		void Stop() noexcept;
		void Poll();

		bool IsConnected() const noexcept;
		bool IsRunning() const noexcept;

		State GetState() const noexcept;
		const std::string& ClientId() const noexcept;
		const std::optional<WillConfig>& GetWill() const noexcept;

		void Publish(const std::string& topic, const std::string& payload, bool retain = false);
		void Subscribe(const std::string& topic_filter, uint8_t qos = 0);
		std::string BuildTopic(const std::string& subtopic) const;
		const std::string& TopicPrefix() const noexcept;

	public:
		// Read-only diagnostics accessors. Safe to call from the HTTP handler:
		// the client is single-threaded and the handler runs on the same
		// io_context thread, so no synchronisation is required.
		const Options::Mqtt::MqttSettings& Settings() const noexcept { return m_Settings; }
		std::size_t PublishQueueDepth() const noexcept { return m_PublishQueue.size(); }
		std::uint16_t ReconnectAttempts() const noexcept { return m_ReconnectAttempts; }
		std::uint64_t PublishedCount() const noexcept { return m_PublishedCount; }
		std::uint64_t DroppedCount() const noexcept { return m_DroppedCount; }
		const std::string& LastError() const noexcept { return m_LastError; }

	public:
		ConnectedSignal OnConnected;
		DisconnectedSignal OnDisconnected;
		MessageReceivedSignal OnMessageReceived;
		ErrorSignal OnError;

	private:
		void PollConnecting();
		void PollSendingConnect();
		void PollWaitingConnack();
		void PollConnected();
		void PollReconnecting();

		// Non-blocking connection establishment (driven by the shared io_context).
		void BeginConnect();
		void OnResolveComplete(uint64_t generation, const boost::system::error_code& ec, const boost::asio::ip::tcp::resolver::results_type& endpoints);
		void OnTcpConnectComplete(uint64_t generation, const boost::system::error_code& ec);
		void OnTlsHandshakeComplete(uint64_t generation, const boost::system::error_code& ec);
		void StartTlsHandshake(uint64_t generation);
		void EnterSendingConnect();

		// Schedule a transition into the Reconnecting state with backoff, replacing
		// the duplicated "CloseSocket + set Reconnecting + schedule + ++attempts"
		// block that previously appeared at every failure site.
		void ScheduleReconnect(const std::string& reason, bool emit_disconnect = false);

		bool DrainWriteBuffer(boost::system::error_code& ec);
		void SendPendingPublishes();
		void ReadIncoming();
		void SendPingreq();

		// MQTT 3.1.1 packet encoding
		std::vector<uint8_t> EncodeConnect();
		std::vector<uint8_t> EncodePublish(const std::string& topic, const std::string& payload, bool retain = false);
		std::vector<uint8_t> EncodeSubscribe(const std::string& topic_filter, uint8_t qos);
		std::vector<uint8_t> EncodePingreq();
		std::vector<uint8_t> EncodeDisconnect();

		// MQTT packet parsing
		bool ParseConnack(const std::vector<uint8_t>& data);

		std::string GenerateClientId() const;
		std::chrono::seconds CalculateReconnectDelay() const;

	private:
		void InitializeSslContext();

		// I/O helpers that work with both plain and TLS sockets
		std::size_t WriteSocket(std::span<const uint8_t> data, boost::system::error_code& ec);
		std::size_t ReadSocket(std::span<uint8_t> buffer, boost::system::error_code& ec);
		void CloseSocket();
		bool IsSocketOpen() const;

		// Test access
		friend class AqualinkAutomate::Test::MqttClientPacketTest;

	private:
		boost::asio::io_context& m_IoContext;
		const Options::Mqtt::MqttSettings m_Settings;

		State m_State{ State::Disconnected };
		bool m_Running{ false };
		std::string m_ClientId;

		// TCP socket (non-blocking)
		std::optional<boost::asio::ip::tcp::socket> m_Socket;

		// Asynchronous connection establishment
		boost::asio::ip::tcp::resolver m_Resolver;
		bool m_ConnectInProgress{ false };
		// Generation token: incremented on every Stop()/reconnect/CloseSocket so that
		// completion handlers from a stale connection attempt are recognised and ignored.
		uint64_t m_ConnectGeneration{ 0 };

		// TLS support
		std::optional<boost::asio::ssl::context> m_SslContext;
		std::optional<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>> m_SslStream;

		// Pending publishes
		struct PendingPublish
		{
			std::string topic;
			std::string payload;
			bool retain{ false };
		};
		std::deque<PendingPublish> m_PublishQueue;

		// Outbound write buffer + offset.  A queued publish (or a control packet) is
		// serialised into m_WriteBuffer and only considered fully sent once
		// m_WriteOffset == m_WriteBuffer.size().  This guards against partial socket
		// writes silently truncating MQTT packets.
		std::vector<uint8_t> m_WriteBuffer;
		std::size_t m_WriteOffset{ 0 };

		// Security: Maximum publish queue size to prevent memory exhaustion
		static constexpr std::size_t MAX_PUBLISH_QUEUE_SIZE = 1000;

		// Read buffer
		std::vector<uint8_t> m_ReadBuffer;
		static constexpr std::size_t READ_CHUNK_SIZE = 4096;

		// Security: Maximum read buffer size to prevent memory exhaustion from malicious brokers
		static constexpr std::size_t MAX_READ_BUFFER_SIZE = 1024 * 1024;  // 1MB

		// Keepalive: the wire value (seconds) is derived from the interval so the two
		// can never drift apart by hand.
		static constexpr auto KEEPALIVE_INTERVAL = std::chrono::seconds(60);
		static constexpr uint16_t KEEPALIVE_SECONDS = static_cast<uint16_t>(KEEPALIVE_INTERVAL.count());
		static_assert(KEEPALIVE_INTERVAL.count() <= 0xFFFF, "MQTT keep-alive must fit in a 16-bit field");

		// How long to wait for a CONNACK after sending CONNECT before giving up.
		static constexpr auto CONNACK_TIMEOUT = std::chrono::seconds(10);

		std::chrono::steady_clock::time_point m_LastPingSent;
		std::chrono::steady_clock::time_point m_LastActivity;

		// Packet ID for Subscribe
		uint16_t m_NextPacketId{ 1 };

		// Reconnection
		uint16_t m_ReconnectAttempts{ 0 };
		std::chrono::steady_clock::time_point m_ReconnectTime;

		// Diagnostics counters (surfaced via /api/diagnostics/mqtt).
		std::uint64_t m_PublishedCount{ 0 };
		std::uint64_t m_DroppedCount{ 0 };
		std::string m_LastError;

		// Last Will and Testament
		std::optional<WillConfig> m_WillConfig;
	};

}
// namespace AqualinkAutomate::Mqtt
