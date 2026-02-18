#pragma once

#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

#include "mqtt/mqtt_client.h"
#include "options/options_mqtt_options.h"

namespace AqualinkAutomate::Test
{

//=============================================================================
// Friend test class for MQTT protocol encoding/parsing and queue inspection.
// Single definition shared across all test TUs to avoid ODR violations.
//=============================================================================

class MqttClientPacketTest
{
public:
	static const std::deque<Mqtt::MqttClient::PendingPublish>& GetPublishQueue(const Mqtt::MqttClient& client)
	{
		return client.m_PublishQueue;
	}

	static std::vector<uint8_t> EncodeConnect(Mqtt::MqttClient& client)
	{
		return client.EncodeConnect();
	}

	static std::vector<uint8_t> EncodePublish(Mqtt::MqttClient& client, const std::string& topic, const std::string& payload, bool retain)
	{
		return client.EncodePublish(topic, payload, retain);
	}

	static std::vector<uint8_t> EncodeDisconnect(Mqtt::MqttClient& client)
	{
		return client.EncodeDisconnect();
	}

	static std::vector<uint8_t> EncodePingreq(Mqtt::MqttClient& client)
	{
		return client.EncodePingreq();
	}

	static bool ParseConnack(Mqtt::MqttClient& client, const std::vector<uint8_t>& data)
	{
		return client.ParseConnack(data);
	}

	/// Force the client into the Connected state for integration testing.
	/// This allows MqttHub publish methods to run without a real broker.
	static void ForceConnectedState(Mqtt::MqttClient& client)
	{
		client.m_State = Mqtt::MqttClient::State::Connected;
		client.m_Running = true;
	}
};

//=============================================================================
// Shared test settings factory
//=============================================================================

inline Options::Mqtt::MqttSettings MakeMqttSettings()
{
	Options::Mqtt::MqttSettings s;
	s.enabled = true;
	s.broker_host = "localhost";
	s.broker_port = 1883;
	s.topic_prefix = "test";
	return s;
}

}
// namespace AqualinkAutomate::Test
