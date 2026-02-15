#include <algorithm>
#include <cctype>
#include <format>

#include "logging/logging.h"
#include "mqtt/ha_discovery.h"
#include "kernel/auxillary_traits/auxillary_traits_helpers.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "version/version_cmake.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Mqtt
{

	HomeAssistantDiscovery::HomeAssistantDiscovery(std::shared_ptr<MqttClient> client, const Options::Mqtt::MqttSettings& settings)
		: m_Client(std::move(client))
		, m_Settings(settings)
	{
		LogInfo(Channel::Mqtt, "Home Assistant Discovery initialized");
	}

	void HomeAssistantDiscovery::ConnectDataHub(const std::shared_ptr<Kernel::DataHub>& data_hub)
	{
		m_DataHub = data_hub;
		LogDebug(Channel::Mqtt, "HA Discovery connected to Data Hub");
	}

	void HomeAssistantDiscovery::PublishDiscoveryConfigs()
	{
		LogInfo(Channel::Mqtt, "Publishing Home Assistant device discovery payload");

		nlohmann::json payload;
		payload["dev"] = BuildDeviceObject();
		payload["o"] = BuildOriginObject();
		payload["availability"] = BuildAvailability();

		nlohmann::json cmps = nlohmann::json::object();
		AddTemperatureSensorComponents(cmps);
		AddChemistrySensorComponents(cmps);
		AddCirculationComponents(cmps);
		AddSystemComponents(cmps);
		AddDynamicDeviceComponents(cmps);
		payload["cmps"] = std::move(cmps);

		auto topic = std::format("{}/device/{}/config",
			m_Settings.ha_discovery_prefix, m_Settings.ha_device_id);
		m_Client->Publish(topic, payload.dump(), /*retain=*/true);

		LogInfo(Channel::Mqtt, "Home Assistant device discovery payload published");
	}

	void HomeAssistantDiscovery::PublishOnline()
	{
		m_Client->Publish(AvailabilityTopic(), "online", /*retain=*/true);
		LogDebug(Channel::Mqtt, "Published HA availability: online");
	}

	void HomeAssistantDiscovery::PublishDeviceStates()
	{
		auto data_hub = m_DataHub.lock();
		if (!data_hub)
		{
			return;
		}

		auto publish_device_state = [&](const std::string& category, const std::shared_ptr<Kernel::AuxillaryDevice>& device)
		{
			if (!device)
			{
				return;
			}

			auto label = device->AuxillaryTraits.TryGet(Kernel::AuxillaryTraitsTypes::LabelTrait{});
			if (!label.has_value())
			{
				return;
			}

			auto slug = Slugify(label.value());
			auto state_topic = m_Client->BuildTopic(std::format("ha/{}_{}", category, slug));
			auto state = std::string(Kernel::AuxillaryTraitsTypes::ConvertStatusToString(device));

			m_Client->Publish(state_topic, state, /*retain=*/true);
		};

		for (const auto& device : data_hub->Pumps())
		{
			publish_device_state("pump", device);
		}

		for (const auto& device : data_hub->Heaters())
		{
			publish_device_state("heater", device);
		}

		for (const auto& device : data_hub->Chlorinators())
		{
			publish_device_state("chlorinator", device);
		}

		for (const auto& device : data_hub->Auxillaries())
		{
			publish_device_state("aux", device);
		}

		LogTrace(Channel::Mqtt, "Published HA device states");
	}

	//=========================================================================
	// Component builders
	//=========================================================================

	void HomeAssistantDiscovery::AddTemperatureSensorComponents(nlohmann::json& cmps)
	{
		auto pool_topic = PoolStatusTopic();

		struct TempSensor
		{
			const char* name;
			const char* key;
			const char* value_template;
		};

		const TempSensor sensors[] = {
			{ "Pool Temperature",           "pool_temp",           "{{ value_json.temperatures.pool.celsius }}" },
			{ "Spa Temperature",            "spa_temp",            "{{ value_json.temperatures.spa.celsius }}" },
			{ "Air Temperature",            "air_temp",            "{{ value_json.temperatures.air.celsius }}" },
			{ "Freeze Protect Temperature", "freeze_protect_temp", "{{ value_json.temperatures.freeze_protect.celsius }}" },
			{ "Pool Setpoint Temperature",  "pool_setpoint_temp",  "{{ value_json.temperatures.pool_setpoint.celsius }}" },
			{ "Spa Setpoint Temperature",   "spa_setpoint_temp",   "{{ value_json.temperatures.spa_setpoint.celsius }}" },
		};

		for (const auto& sensor : sensors)
		{
			cmps[sensor.key] = {
				{"p", "sensor"},
				{"name", sensor.name},
				{"unique_id", UniqueId(sensor.key)},
				{"state_topic", pool_topic},
				{"value_template", sensor.value_template},
				{"device_class", "temperature"},
				{"unit_of_measurement", "\u00B0C"},
				{"state_class", "measurement"}
			};
		}
	}

	void HomeAssistantDiscovery::AddChemistrySensorComponents(nlohmann::json& cmps)
	{
		auto pool_topic = PoolStatusTopic();

		cmps["orp"] = {
			{"p", "sensor"},
			{"name", "ORP"},
			{"unique_id", UniqueId("orp")},
			{"state_topic", pool_topic},
			{"value_template", "{{ value_json.chemistry.orp.value_mv }}"},
			{"device_class", "voltage"},
			{"unit_of_measurement", "mV"},
			{"state_class", "measurement"}
		};

		cmps["ph"] = {
			{"p", "sensor"},
			{"name", "pH"},
			{"unique_id", UniqueId("ph")},
			{"state_topic", pool_topic},
			{"value_template", "{{ value_json.chemistry.ph.value }}"},
			{"device_class", "ph"},
			{"state_class", "measurement"}
		};

		cmps["salt_level"] = {
			{"p", "sensor"},
			{"name", "Salt Level"},
			{"unique_id", UniqueId("salt_level")},
			{"state_topic", pool_topic},
			{"value_template", "{{ value_json.chemistry.salt.value_ppm }}"},
			{"unit_of_measurement", "ppm"},
			{"state_class", "measurement"}
		};
	}

	void HomeAssistantDiscovery::AddCirculationComponents(nlohmann::json& cmps)
	{
		auto pool_topic = PoolStatusTopic();

		cmps["circulation_mode"] = {
			{"p", "sensor"},
			{"name", "Circulation Mode"},
			{"unique_id", UniqueId("circulation_mode")},
			{"state_topic", pool_topic},
			{"value_template", "{{ value_json.circulation.mode }}"}
		};

		cmps["spa_mode"] = {
			{"p", "binary_sensor"},
			{"name", "Spa Mode"},
			{"unique_id", UniqueId("spa_mode")},
			{"state_topic", pool_topic},
			{"value_template", "{{ value_json.circulation.spa_mode }}"},
			{"payload_on", "true"},
			{"payload_off", "false"}
		};

		cmps["clean_mode"] = {
			{"p", "binary_sensor"},
			{"name", "Clean Mode"},
			{"unique_id", UniqueId("clean_mode")},
			{"state_topic", pool_topic},
			{"value_template", "{{ value_json.circulation.clean_mode }}"},
			{"payload_on", "true"},
			{"payload_off", "false"}
		};
	}

	void HomeAssistantDiscovery::AddSystemComponents(nlohmann::json& cmps)
	{
		auto system_topic = SystemStatusTopic();

		cmps["uptime"] = {
			{"p", "sensor"},
			{"name", "Uptime"},
			{"unique_id", UniqueId("uptime")},
			{"state_topic", system_topic},
			{"value_template", "{{ value_json.uptime_seconds }}"},
			{"device_class", "duration"},
			{"unit_of_measurement", "s"},
			{"state_class", "total_increasing"},
			{"entity_category", "diagnostic"}
		};
	}

	void HomeAssistantDiscovery::AddDynamicDeviceComponents(nlohmann::json& cmps)
	{
		auto data_hub = m_DataHub.lock();
		if (!data_hub)
		{
			return;
		}

		auto add_binary_sensor = [&](const std::string& category, const std::shared_ptr<Kernel::AuxillaryDevice>& dev,
			const std::string& payload_on, const std::string& payload_off)
		{
			if (!dev)
			{
				return;
			}

			auto label = dev->AuxillaryTraits.TryGet(Kernel::AuxillaryTraitsTypes::LabelTrait{});
			if (!label.has_value())
			{
				return;
			}

			auto slug = Slugify(label.value());
			auto key = std::format("{}_{}", category, slug);
			auto state_topic = m_Client->BuildTopic(std::format("ha/{}", key));

			cmps[key] = {
				{"p", "binary_sensor"},
				{"name", label.value()},
				{"unique_id", UniqueId(key)},
				{"state_topic", state_topic},
				{"payload_on", payload_on},
				{"payload_off", payload_off}
			};
		};

		auto add_sensor = [&](const std::string& category, const std::shared_ptr<Kernel::AuxillaryDevice>& dev)
		{
			if (!dev)
			{
				return;
			}

			auto label = dev->AuxillaryTraits.TryGet(Kernel::AuxillaryTraitsTypes::LabelTrait{});
			if (!label.has_value())
			{
				return;
			}

			auto slug = Slugify(label.value());
			auto key = std::format("{}_{}", category, slug);
			auto state_topic = m_Client->BuildTopic(std::format("ha/{}", key));

			cmps[key] = {
				{"p", "sensor"},
				{"name", label.value()},
				{"unique_id", UniqueId(key)},
				{"state_topic", state_topic}
			};
		};

		// Pumps -> binary_sensor (Running / Off)
		for (const auto& dev : data_hub->Pumps())
		{
			add_binary_sensor("pump", dev, "Running", "Off");
		}

		// Chlorinators -> binary_sensor (Running / Off)
		for (const auto& dev : data_hub->Chlorinators())
		{
			add_binary_sensor("chlorinator", dev, "Running", "Off");
		}

		// Auxiliaries -> binary_sensor (On / Off)
		for (const auto& dev : data_hub->Auxillaries())
		{
			add_binary_sensor("aux", dev, "On", "Off");
		}

		// Heaters -> sensor (multi-state: Off/Heating/Enabled)
		for (const auto& dev : data_hub->Heaters())
		{
			add_sensor("heater", dev);
		}
	}

	//=========================================================================
	// Helpers
	//=========================================================================

	nlohmann::json HomeAssistantDiscovery::BuildDeviceObject() const
	{
		nlohmann::json device = {
			{"identifiers", nlohmann::json::array({m_Settings.ha_device_id})},
			{"name", "aqualink-automate"},
			{"manufacturer", "Jandy / Zodiac"},
			{"sw_version", Version::VersionInfo::ProjectVersion()}
		};

		if (auto data_hub = m_DataHub.lock())
		{
			auto model = data_hub->EquipmentVersions.ModelNumber();
			if (!model.empty())
			{
				device["model"] = model;
			}

			auto firmware = data_hub->EquipmentVersions.FirmwareRevision();
			if (!firmware.empty())
			{
				device["hw_version"] = firmware;
			}
		}

		return device;
	}

	nlohmann::json HomeAssistantDiscovery::BuildOriginObject() const
	{
		return {
			{"name", "aqualink-automate"},
			{"sw_version", Version::VersionInfo::ProjectVersion()},
			{"support_url", Version::VersionInfo::ProjectHomepageURL()}
		};
	}

	nlohmann::json HomeAssistantDiscovery::BuildAvailability() const
	{
		return nlohmann::json::array({
			{{"topic", AvailabilityTopic()}, {"payload_available", "online"}, {"payload_not_available", "offline"}}
		});
	}

	std::string HomeAssistantDiscovery::Slugify(const std::string& input)
	{
		std::string result;
		result.reserve(input.size());

		for (char c : input)
		{
			if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
			{
				result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			}
			else if (c == ' ' || c == '-' || c == '.')
			{
				if (!result.empty() && result.back() != '_')
				{
					result += '_';
				}
			}
		}

		// Trim trailing underscore
		if (!result.empty() && result.back() == '_')
		{
			result.pop_back();
		}

		return result;
	}

	std::string HomeAssistantDiscovery::UniqueId(const std::string& suffix) const
	{
		return std::format("{}_{}", m_Settings.ha_device_id, suffix);
	}

	std::string HomeAssistantDiscovery::AvailabilityTopic() const
	{
		return m_Client->BuildTopic("status/availability");
	}

	std::string HomeAssistantDiscovery::PoolStatusTopic() const
	{
		return m_Client->BuildTopic("status/pool");
	}

	std::string HomeAssistantDiscovery::SystemStatusTopic() const
	{
		return m_Client->BuildTopic("status/system");
	}

}
// namespace AqualinkAutomate::Mqtt
