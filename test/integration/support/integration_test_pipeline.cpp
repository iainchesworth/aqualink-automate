#include <boost/test/unit_test.hpp>

#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/statistics_hub.h"
#include "mqtt/mqtt_hub.h"
#include "mqtt/mqtt_client.h"
#include "http/server/routing/routing.h"
#include "support/unit_test_mqtt_support.h"
#include "support/unit_test_httprequestresponse.h"
#include "support/integration_test_pipeline.h"

namespace AqualinkAutomate::Test
{

	IntegrationPipeline::IntegrationPipeline()
		: m_IoContext()
		, m_HubLocator()
	{
		// Extract shared_ptrs from the hub locator (which creates them in its constructor)
		m_DataHub = m_HubLocator.Find<Kernel::DataHub>();
		m_EquipmentHub = m_HubLocator.Find<Kernel::EquipmentHub>();
		m_StatisticsHub = m_HubLocator.Find<Kernel::StatisticsHub>();

		// Create MqttHub with test settings
		auto mqtt_settings = MakeMqttSettings();
		m_MqttHub = std::make_shared<Mqtt::MqttHub>(m_IoContext, mqtt_settings);

		// Connect hubs to MqttHub (mirrors production wiring)
		m_MqttHub->ConnectDataHub(m_DataHub);
		m_MqttHub->ConnectEquipmentHub(m_EquipmentHub);
		m_MqttHub->ConnectStatisticsHub(m_StatisticsHub);
	}

	IntegrationPipeline::~IntegrationPipeline() = default;

	std::shared_ptr<Kernel::DataHub> IntegrationPipeline::DataHub() const
	{
		return m_DataHub;
	}

	std::shared_ptr<Kernel::EquipmentHub> IntegrationPipeline::EquipmentHub() const
	{
		return m_EquipmentHub;
	}

	std::shared_ptr<Kernel::StatisticsHub> IntegrationPipeline::StatisticsHub() const
	{
		return m_StatisticsHub;
	}

	void IntegrationPipeline::AddDevice(std::shared_ptr<Kernel::AuxillaryDevice> device)
	{
		m_DataHub->Devices.Add(device);
	}

	void IntegrationPipeline::PublishAllMqttStatus()
	{
		m_MqttCapture.Clear();

		auto client = m_MqttHub->GetMqttClient();

		// Force the client into Connected state so MqttHub::IsRunning() returns true.
		// This allows the hub's publish methods to execute without a real broker.
		MqttClientPacketTest::ForceConnectedState(*client);

		// Start the hub (sets m_Running on the hub side).
		m_MqttHub->Start();

		// Now PublishAllStatus() will pass the IsRunning() check and serialize+enqueue.
		m_MqttHub->PublishAllStatus();

		// Extract queued messages from the client's publish queue.
		const auto& queue = MqttClientPacketTest::GetPublishQueue(*client);
		for (const auto& pending : queue)
		{
			m_MqttCapture.Capture(pending.topic, pending.payload, pending.retain);
		}
	}

	const MqttPublishCapture& IntegrationPipeline::MqttCapture() const
	{
		return m_MqttCapture;
	}

	void IntegrationPipeline::ClearMqttCapture()
	{
		m_MqttCapture.Clear();
	}

	void IntegrationPipeline::InjectMqttCommand(const std::string& command, const nlohmann::json& payload)
	{
		// Simulate the MQTT message reception by constructing the full command topic
		// and calling the MqttHub's command processing pathway.
		if (m_MqttHub->HasCommand(command))
		{
			// Reconstruct the full topic as it would appear from the broker.
			auto full_topic = m_MqttHub->CommandTopic(command);

			// Invoke the registered command handler directly.
			// The handlers are registered in m_CommandHandlers; we access via the public API.
			// Note: ProcessCommand is private, so we invoke HandleMessage indirectly
			// by triggering the client's OnMessageReceived signal.
			auto client = m_MqttHub->GetMqttClient();
			client->OnMessageReceived(full_topic, payload.dump());
		}
	}

	HTTP::Response IntegrationPipeline::PerformHttpGet(const std::string_view& path)
	{
		return Test::PerformHttpRequestResponse(path);
	}

	void IntegrationPipeline::PollIoContext()
	{
		m_IoContext.poll();
		m_IoContext.restart();
	}

}
// namespace AqualinkAutomate::Test
