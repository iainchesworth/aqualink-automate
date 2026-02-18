#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace AqualinkAutomate::Test
{

	struct MqttCapturedMessage
	{
		std::string topic;
		std::string payload;
		bool retain{ false };
		std::chrono::system_clock::time_point timestamp;
	};

	/// Lightweight MQTT publish capture for integration tests.
	/// Stores all messages published via MqttClient::Publish() for assertion.
	class MqttPublishCapture
	{
	public:
		void Capture(const std::string& topic, const std::string& payload, bool retain)
		{
			m_Messages.push_back({ topic, payload, retain, std::chrono::system_clock::now() });
		}

		const std::vector<MqttCapturedMessage>& Messages() const
		{
			return m_Messages;
		}

		std::vector<MqttCapturedMessage> MessagesForTopic(const std::string& topic_substring) const
		{
			std::vector<MqttCapturedMessage> result;
			for (const auto& msg : m_Messages)
			{
				if (msg.topic.find(topic_substring) != std::string::npos)
				{
					result.push_back(msg);
				}
			}
			return result;
		}

		bool HasMessageForTopic(const std::string& topic_substring) const
		{
			for (const auto& msg : m_Messages)
			{
				if (msg.topic.find(topic_substring) != std::string::npos)
				{
					return true;
				}
			}
			return false;
		}

		std::size_t Count() const { return m_Messages.size(); }

		void Clear() { m_Messages.clear(); }

	private:
		std::vector<MqttCapturedMessage> m_Messages;
	};

}
// namespace AqualinkAutomate::Test
