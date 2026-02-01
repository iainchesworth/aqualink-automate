#pragma once

#include <memory>

#include <boost/asio/io_context.hpp>
#include <nlohmann/json.hpp>

#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/hub_locator.h"
#include "kernel/statistics_hub.h"
#include "mqtt/mqtt_hub.h"
#include "options/options_mqtt_options.h"

namespace AqualinkAutomate::Mqtt
{

	/// High-level MQTT integration class that orchestrates MQTT functionality.
	/// Provides a simple interface for the main application to enable MQTT
	/// data publishing and command handling.
	class MqttIntegration
	{
	public:
		explicit MqttIntegration(boost::asio::io_context& io_context, const Options::Mqtt::MqttSettings& settings);
		~MqttIntegration() = default;

		// Non-copyable, non-movable
		MqttIntegration(const MqttIntegration&) = delete;
		MqttIntegration& operator=(const MqttIntegration&) = delete;
		MqttIntegration(MqttIntegration&&) = delete;
		MqttIntegration& operator=(MqttIntegration&&) = delete;

	public:
		//---------------------------------------------------------------------
		// LIFECYCLE
		//---------------------------------------------------------------------

		/// Start the MQTT integration. Only starts if enabled in settings.
		void Start();

		/// Stop the MQTT integration.
		void Stop();

		/// Poll the MQTT integration (forwards to hub).
		void Poll();

		/// Check if MQTT is enabled in settings.
		bool IsEnabled() const;

		/// Check if MQTT is currently running and connected.
		bool IsRunning() const;

		//---------------------------------------------------------------------
		// HUB CONNECTIONS
		//---------------------------------------------------------------------

		/// Connect to all hubs using the hub locator.
		void ConnectHubs(Kernel::HubLocator& hub_locator);

		/// Connect to individual hubs.
		void ConnectHubs(
			std::shared_ptr<Kernel::DataHub> data_hub,
			std::shared_ptr<Kernel::EquipmentHub> equipment_hub,
			std::shared_ptr<Kernel::StatisticsHub> statistics_hub);

		//---------------------------------------------------------------------
		// ACCESS
		//---------------------------------------------------------------------

		/// Get the underlying MQTT hub for advanced use cases.
		std::shared_ptr<MqttHub> GetMqttHub() const;

	private:
		//---------------------------------------------------------------------
		// DEFAULT COMMAND HANDLERS
		//---------------------------------------------------------------------

		void RegisterDefaultCommands();

	private:
		const Options::Mqtt::MqttSettings m_Settings;
		std::shared_ptr<MqttHub> m_Hub;

		// Weak references to connected hubs
		std::weak_ptr<Kernel::DataHub> m_DataHub;
		std::weak_ptr<Kernel::EquipmentHub> m_EquipmentHub;
		std::weak_ptr<Kernel::StatisticsHub> m_StatisticsHub;
	};

	/// Factory function to create MqttIntegration from settings.
	/// Returns nullptr if MQTT is not enabled.
	std::shared_ptr<MqttIntegration> CreateMqttIntegration(boost::asio::io_context& io_context, const Options::Mqtt::MqttSettings& settings);

}
// namespace AqualinkAutomate::Mqtt
