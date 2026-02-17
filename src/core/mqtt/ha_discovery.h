#pragma once

#include <memory>
#include <string>
#include <vector>

#include <boost/signals2.hpp>
#include <nlohmann/json.hpp>

#include "kernel/data_hub.h"
#include "mqtt/mqtt_client.h"
#include "options/options_mqtt_options.h"


namespace AqualinkAutomate::Mqtt
{

	/// Publishes Home Assistant MQTT Discovery configuration payloads so that
	/// HA auto-discovers sensors and binary_sensors for the pool controller.
	///
	/// Fixed sensors point HA at granular topics:
	///   {prefix}/pool/temperatures, {prefix}/pool/chemistry,
	///   {prefix}/pool/circulation, {prefix}/system/status.
	/// Dynamic devices (pumps, heaters, etc.) get per-device state topics at
	/// {prefix}/ha/{slug}.
	class HomeAssistantDiscovery
	{
	public:
		HomeAssistantDiscovery(std::shared_ptr<MqttClient> client, const Options::Mqtt::MqttSettings& settings);

		/// Store a weak reference to the data hub for reading device info.
		void ConnectDataHub(const std::shared_ptr<Kernel::DataHub>& data_hub);

		/// Publish all discovery configs (fixed sensors + dynamic devices).
		void PublishDiscoveryConfigs();

		/// Publish "online" (retained) to the availability topic.
		void PublishOnline();

		/// Publish current state for each dynamic device to per-device topics.
		void PublishDeviceStates();

		//---------------------------------------------------------------------
		// HELPERS (public for testability)
		//---------------------------------------------------------------------

		/// Build the shared HA "device" object for all entities.
		nlohmann::json BuildDeviceObject() const;

		/// Build the "origin" object.
		nlohmann::json BuildOriginObject() const;

		/// Build the availability array.
		nlohmann::json BuildAvailability() const;

		/// Convert a label like "Filter Pump" to "filter_pump".
		static std::string Slugify(const std::string& input);

		/// Build a unique ID: "aqualink_{prefix}_{suffix}".
		std::string UniqueId(const std::string& suffix) const;

		/// Get the availability topic.
		std::string AvailabilityTopic() const;

		/// Get the pool temperatures topic.
		std::string TemperaturesTopic() const;

		/// Get the pool chemistry topic.
		std::string ChemistryTopic() const;

		/// Get the pool circulation topic.
		std::string CirculationTopic() const;

		/// Get the system status topic.
		std::string SystemStatusTopic() const;

	private:
		//---------------------------------------------------------------------
		// COMPONENT BUILDERS (populate cmps JSON object)
		//---------------------------------------------------------------------

		void AddTemperatureSensorComponents(nlohmann::json& cmps);
		void AddSetpointComponents(nlohmann::json& cmps);
		void AddChemistrySensorComponents(nlohmann::json& cmps);
		void AddCirculationComponents(nlohmann::json& cmps);
		void AddSystemComponents(nlohmann::json& cmps);
		void AddDynamicDeviceComponents(nlohmann::json& cmps);

		std::string SetpointCommandTopic(const std::string& target) const;
		std::string DeviceCommandTopic(const std::string& slug) const;

	private:
		std::shared_ptr<MqttClient> m_Client;
		Options::Mqtt::MqttSettings m_Settings;
		std::weak_ptr<Kernel::DataHub> m_DataHub;
	};

}
// namespace AqualinkAutomate::Mqtt
