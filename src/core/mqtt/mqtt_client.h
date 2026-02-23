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

	/// Poll-based MQTT 3.1.1 client with synchronous state machine.
	///
	/// Thread-safety contract: This class is NOT thread-safe. All public methods
	/// (including Publish(), Poll(), Start(), Stop()) must be called from the
	/// same thread that runs the associated io_context. The internal publish
	/// queue and connection state are accessed without synchronization,
	/// relying on single-threaded execution within the io_context poll loop.
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
		std::size_t WriteSocket(const std::vector<uint8_t>& data, boost::system::error_code& ec);
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

		// Security: Maximum publish queue size to prevent memory exhaustion
		static constexpr std::size_t MAX_PUBLISH_QUEUE_SIZE = 1000;

		// Read buffer
		std::vector<uint8_t> m_ReadBuffer;
		static constexpr std::size_t READ_CHUNK_SIZE = 4096;

		// Security: Maximum read buffer size to prevent memory exhaustion from malicious brokers
		static constexpr std::size_t MAX_READ_BUFFER_SIZE = 1024 * 1024;  // 1MB

		// Keepalive
		std::chrono::steady_clock::time_point m_LastPingSent;
		std::chrono::steady_clock::time_point m_LastActivity;
		static constexpr auto KEEPALIVE_INTERVAL = std::chrono::seconds(60);
		static constexpr uint16_t KEEPALIVE_SECONDS = 60;

		// Packet ID for Subscribe
		uint16_t m_NextPacketId{ 1 };

		// Reconnection
		uint16_t m_ReconnectAttempts{ 0 };
		std::chrono::steady_clock::time_point m_ReconnectTime;

		// Last Will and Testament
		std::optional<WillConfig> m_WillConfig;
	};

}
// namespace AqualinkAutomate::Mqtt
