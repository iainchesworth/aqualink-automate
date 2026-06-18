#pragma once

#include <deque>

#include "mqtt/mqtt_client.h"
#include "options/options_mqtt_options.h"

namespace AqualinkAutomate::Test
{

//=============================================================================
// Friend test seam for the MQTT client: publish-queue inspection and forcing
// the connected state. A single definition shared across all test TUs to avoid
// ODR violations.
//
// (Wire-format / packet-encoding is now owned by the async_mqtt library and is
// no longer hand-rolled here, so the former Encode*/Parse* accessors are gone.)
//=============================================================================

class MqttClientPacketTest
{
public:
	/// Inspect the pending outbound publish queue (topics/payloads/retain) that
	/// the hub / HA-discovery paths enqueued.
	static const std::deque<Mqtt::MqttClient::PendingPublish>& GetPublishQueue(const Mqtt::MqttClient& client)
	{
		return client.m_PublishQueue;
	}

	/// Force the client into the Connected state for integration testing, so the
	/// MqttHub / HA-discovery publish paths run (and enqueue) without a real
	/// broker. The queued publishes can then be inspected via GetPublishQueue().
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
