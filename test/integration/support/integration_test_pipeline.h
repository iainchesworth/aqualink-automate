#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <nlohmann/json.hpp>

#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/statistics_hub.h"
#include "mqtt/mqtt_hub.h"
#include "mqtt/mqtt_integration.h"
#include "http/server/server_types.h"
#include "interfaces/icommanddispatcher.h"
#include "support/unit_test_hublocatorinjector.h"
#include "support/integration_test_mqtt_capture.h"

namespace AqualinkAutomate::Test
{

	/// Central integration test fixture that wires the full component chain:
	///
	///   DataHub ← direct manipulation (temperature, chemistry, devices)
	///   DataHub → MqttHub (publish captured via MqttPublishCapture)
	///   DataHub → WebRoutes (HTTP request exercising)
	///   CommandDispatcher → DataHub (command routing)
	///
	/// This fixture creates real hub instances and connects them through the
	/// same signal pathways used in production, enabling end-to-end validation
	/// without a real serial port or MQTT broker.
	class IntegrationPipeline
	{
	public:
		IntegrationPipeline();
		virtual ~IntegrationPipeline();

		//---------------------------------------------------------------------
		// HUB ACCESS
		//---------------------------------------------------------------------

		std::shared_ptr<Kernel::DataHub> DataHub() const;
		std::shared_ptr<Kernel::EquipmentHub> EquipmentHub() const;
		std::shared_ptr<Kernel::StatisticsHub> StatisticsHub() const;

		//---------------------------------------------------------------------
		// DEVICE POPULATION
		//---------------------------------------------------------------------

		/// Add an auxillary device directly to the DataHub device graph.
		void AddDevice(std::shared_ptr<Kernel::AuxillaryDevice> device);

		//---------------------------------------------------------------------
		// MQTT INTERACTION
		//---------------------------------------------------------------------

		/// Trigger an MQTT publish cycle (publishes all current status).
		void PublishAllMqttStatus();

		/// Get all captured MQTT messages.
		const MqttPublishCapture& MqttCapture() const;

		/// Clear captured MQTT messages.
		void ClearMqttCapture();

		/// Simulate an incoming MQTT command (calls MqttHub command handler).
		void InjectMqttCommand(const std::string& command, const nlohmann::json& payload);

		//---------------------------------------------------------------------
		// HTTP INTERACTION
		//---------------------------------------------------------------------

		/// Perform an HTTP GET request through the routing system.
		HTTP::Response PerformHttpGet(const std::string_view& path);

		//---------------------------------------------------------------------
		// IO CONTEXT
		//---------------------------------------------------------------------

		/// Poll the io_context to process pending asynchronous work.
		void PollIoContext();

	// Protected so test fixtures can access the hub locator for route registration
	public:
		HubLocatorInjector m_HubLocator;

	private:
		boost::asio::io_context m_IoContext;

		std::shared_ptr<Kernel::DataHub> m_DataHub;
		std::shared_ptr<Kernel::EquipmentHub> m_EquipmentHub;
		std::shared_ptr<Kernel::StatisticsHub> m_StatisticsHub;

		std::shared_ptr<Mqtt::MqttHub> m_MqttHub;
		MqttPublishCapture m_MqttCapture;
	};

}
// namespace AqualinkAutomate::Test
