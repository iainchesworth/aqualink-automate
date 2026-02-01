#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/signals2.hpp>

#include "options/options_mqtt_options.h"

namespace AqualinkAutomate::Mqtt
{

	/// Poll-based MQTT 3.1.1 client with synchronous state machine.
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
		void Start();
		void Stop();
		void Poll();

		bool IsConnected() const noexcept;
		bool IsRunning() const noexcept;

		void Publish(const std::string& topic, const std::string& payload);
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
		std::vector<uint8_t> EncodePublish(const std::string& topic, const std::string& payload);
		std::vector<uint8_t> EncodePingreq();
		std::vector<uint8_t> EncodeDisconnect();

		// MQTT packet parsing
		bool ParseConnack(const std::vector<uint8_t>& data);

		std::string GenerateClientId() const;
		std::chrono::seconds CalculateReconnectDelay() const;

	private:
		boost::asio::io_context& m_IoContext;
		const Options::Mqtt::MqttSettings m_Settings;

		State m_State{ State::Disconnected };
		bool m_Running{ false };
		std::string m_ClientId;

		// TCP socket (non-blocking)
		std::optional<boost::asio::ip::tcp::socket> m_Socket;

		// Pending publishes
		struct PendingPublish
		{
			std::string topic;
			std::string payload;
		};
		std::deque<PendingPublish> m_PublishQueue;

		// Read buffer
		std::vector<uint8_t> m_ReadBuffer;
		static constexpr std::size_t READ_CHUNK_SIZE = 4096;

		// Keepalive
		std::chrono::steady_clock::time_point m_LastPingSent;
		std::chrono::steady_clock::time_point m_LastActivity;
		static constexpr auto KEEPALIVE_INTERVAL = std::chrono::seconds(60);
		static constexpr uint16_t KEEPALIVE_SECONDS = 60;

		// Reconnection
		uint16_t m_ReconnectAttempts{ 0 };
		std::chrono::steady_clock::time_point m_ReconnectTime;
	};

}
// namespace AqualinkAutomate::Mqtt
