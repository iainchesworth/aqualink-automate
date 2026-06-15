#include <algorithm>
#include <format>
#include <memory>
#include <optional>
#include <string>

#include "alerting/alert_condition.h"
#include "kernel/auxillary_traits/auxillary_traits_helpers.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "logging/logging.h"
#include "mqtt/ha_discovery.h"
#include "mqtt/mqtt_topic_scheme.h"
#include "utility/slugify.h"
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
		// Guard the whole discovery build/publish: a single bad device (e.g. a JSON
		// build throwing) must not abort discovery for every entity, which would surface
		// in Home Assistant as silent/unavailable entities with no diagnostic trail.
		try
		{
			LogDebug(Channel::Mqtt, "Publishing Home Assistant device discovery payload");

			nlohmann::json payload;
			payload["dev"] = BuildDeviceObject();
			payload["o"] = BuildOriginObject();
			payload["availability"] = BuildAvailability();

			nlohmann::json cmps = nlohmann::json::object();
			AddTemperatureSensorComponents(cmps);
			AddSetpointComponents(cmps);
			AddChemistrySensorComponents(cmps);
			AddCirculationComponents(cmps);
			AddSystemComponents(cmps);
			AddAlertComponents(cmps);
			AddDynamicDeviceComponents(cmps);
			payload["cmps"] = std::move(cmps);

			auto topic = std::format("{}/device/{}/config",
				m_Settings.ha_discovery_prefix, m_Settings.ha_device_id);
			m_Client->Publish(topic, payload.dump(), /*retain=*/true);

			LogDebug(Channel::Mqtt, "Home Assistant device discovery payload published");
		}
		catch (const std::exception& ex)
		{
			LogError(Channel::Mqtt, [&] { return std::format("Failed to publish HA discovery configs: {}", ex.what()); });
		}
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

		// Guard the whole sweep: a malformed device must not abort state publishing for
		// the remaining devices (which would leave their HA entities stuck unavailable).
		try
		{
			std::size_t device_count = 0;

			auto publish_device_state = [&](TopicScheme::DeviceCategory category, const std::shared_ptr<Kernel::AuxillaryDevice>& device)
			{
				if (!device)
				{
					return;
				}

				auto label = device->AuxillaryTraits.TryGet(Kernel::AuxillaryTraitsTypes::LabelTrait{});
				if (!label.has_value())
				{
					LogDebug(Channel::Mqtt, [&] { return std::format("Skipping HA state for {} device with no label trait", TopicScheme::CategoryName(category)); });
					return;
				}

				auto slug = Slugify(label.value());
				auto state_topic = m_Client->BuildTopic(TopicScheme::DeviceStateSubtopic(category, slug));
				auto state = std::string(Kernel::AuxillaryTraitsTypes::ConvertStatusToString(device));

				m_Client->Publish(state_topic, state, /*retain=*/true);
				++device_count;
			};

			for (const auto& device : data_hub->Pumps())
			{
				publish_device_state(TopicScheme::DeviceCategory::Pump, device);
			}

			for (const auto& device : data_hub->Heaters())
			{
				publish_device_state(TopicScheme::DeviceCategory::Heater, device);
			}

			for (const auto& device : data_hub->Chlorinators())
			{
				publish_device_state(TopicScheme::DeviceCategory::Chlorinator, device);
			}

			for (const auto& device : data_hub->Auxillaries())
			{
				publish_device_state(TopicScheme::DeviceCategory::Auxillary, device);
			}

			LogTrace(Channel::Mqtt, [&] { return std::format("Published HA device states ({} devices)", device_count); });
		}
		catch (const std::exception& ex)
		{
			LogError(Channel::Mqtt, [&] { return std::format("Failed to publish HA device states: {}", ex.what()); });
		}
	}

	//=========================================================================
	// Component builders
	//=========================================================================

	void HomeAssistantDiscovery::AddTemperatureSensorComponents(nlohmann::json& cmps)
	{
		auto temperatures_topic = TemperaturesTopic();

		struct TempSensor
		{
			const char* name;
			const char* key;
			const char* value_template;
		};

		const TempSensor sensors[] = {
			{ "Pool Temperature",           "pool_temp",           "{{ value_json.pool.celsius if value_json.pool else '' }}" },
			{ "Spa Temperature",            "spa_temp",            "{{ value_json.spa.celsius if value_json.spa else '' }}" },
			{ "Air Temperature",            "air_temp",            "{{ value_json.air.celsius if value_json.air else '' }}" },
			{ "Freeze Protect Temperature", "freeze_protect_temp", "{{ value_json.freeze_protect.celsius if value_json.freeze_protect else '' }}" },
		};

		for (const auto& sensor : sensors)
		{
			cmps[sensor.key] = {
				{"p", "sensor"},
				{"name", sensor.name},
				{"unique_id", UniqueId(sensor.key)},
				{"state_topic", temperatures_topic},
				{"value_template", sensor.value_template},
				{"device_class", "temperature"},
				{"unit_of_measurement", "\u00B0C"},
				{"state_class", "measurement"}
			};
		}
	}

	void HomeAssistantDiscovery::AddSetpointComponents(nlohmann::json& cmps)
	{
		auto temperatures_topic = TemperaturesTopic();

		struct SetpointEntity
		{
			const char* name;
			const char* key;
			const char* value_template;
			const char* target;
		};

		const SetpointEntity entities[] = {
			{ "Pool Setpoint", "pool_setpoint", "{{ value_json.pool_setpoint.celsius if value_json.pool_setpoint else '' }}", "pool" },
			{ "Spa Setpoint",  "spa_setpoint",  "{{ value_json.spa_setpoint.celsius if value_json.spa_setpoint else '' }}",  "spa" },
		};

		for (const auto& entity : entities)
		{
			cmps[entity.key] = {
				{"p", "number"},
				{"name", entity.name},
				{"unique_id", UniqueId(entity.key)},
				{"state_topic", temperatures_topic},
				{"value_template", entity.value_template},
				{"command_topic", SetpointCommandTopic(entity.target)},
				{"min", 15},
				{"max", 41},
				{"step", 0.5},
				{"unit_of_measurement", "\u00B0C"},
				{"device_class", "temperature"},
				{"mode", "slider"}
			};
		}
	}

	void HomeAssistantDiscovery::AddChemistrySensorComponents(nlohmann::json& cmps)
	{
		auto chemistry_topic = ChemistryTopic();

		cmps["orp"] = {
			{"p", "sensor"},
			{"name", "ORP"},
			{"unique_id", UniqueId("orp")},
			{"state_topic", chemistry_topic},
			{"value_template", "{{ value_json.orp.value_mv }}"},
			{"device_class", "voltage"},
			{"unit_of_measurement", "mV"},
			{"state_class", "measurement"}
		};

		cmps["ph"] = {
			{"p", "sensor"},
			{"name", "pH"},
			{"unique_id", UniqueId("ph")},
			{"state_topic", chemistry_topic},
			{"value_template", "{{ value_json.ph.value }}"},
			{"device_class", "ph"},
			{"state_class", "measurement"}
		};

		cmps["salt_level"] = {
			{"p", "sensor"},
			{"name", "Salt Level"},
			{"unique_id", UniqueId("salt_level")},
			{"state_topic", chemistry_topic},
			{"value_template", "{{ value_json.salt.value_ppm }}"},
			{"unit_of_measurement", "ppm"},
			{"state_class", "measurement"}
		};
	}

	void HomeAssistantDiscovery::AddCirculationComponents(nlohmann::json& cmps)
	{
		auto circulation_topic = CirculationTopic();

		cmps["circulation_mode"] = {
			{"p", "sensor"},
			{"name", "Circulation Mode"},
			{"unique_id", UniqueId("circulation_mode")},
			{"state_topic", circulation_topic},
			{"value_template", "{{ value_json.mode }}"}
		};

		// Controller operating mode (Normal / Service / TimeOut) from the configuration topic.
		// Lets an HA-only user see when the panel is in Service or Timeout (no control possible).
		cmps["equipment_mode"] = {
			{"p", "sensor"},
			{"name", "Equipment Mode"},
			{"unique_id", UniqueId("equipment_mode")},
			{"state_topic", ConfigurationTopic()},
			{"value_template", "{{ value_json.equipment_mode }}"}
		};

		cmps["spa_mode"] = {
			{"p", "binary_sensor"},
			{"name", "Spa Mode"},
			{"unique_id", UniqueId("spa_mode")},
			{"state_topic", circulation_topic},
			{"value_template", "{{ value_json.spa_mode }}"},
			{"payload_on", "true"},
			{"payload_off", "false"}
		};

		cmps["clean_mode"] = {
			{"p", "binary_sensor"},
			{"name", "Clean Mode"},
			{"unique_id", UniqueId("clean_mode")},
			{"state_topic", circulation_topic},
			{"value_template", "{{ value_json.clean_mode }}"},
			{"payload_on", "true"},
			{"payload_off", "false"}
		};

		// Circulation mode select — only for combo/dual systems.
		auto data_hub = m_DataHub.lock();
		if (data_hub
			&& (data_hub->PoolConfiguration == Kernel::PoolConfigurations::DualBody_SharedEquipment
				|| data_hub->PoolConfiguration == Kernel::PoolConfigurations::DualBody_DualEquipment))
		{
			cmps["circulation_mode_select"] = {
				{"p", "select"},
				{"name", "Circulation Mode"},
				{"unique_id", UniqueId("circulation_mode_select")},
				{"state_topic", circulation_topic},
				{"value_template", "{{ value_json.mode }}"},
				{"command_topic", CirculationCommandTopic()},
				{"options", nlohmann::json::array({"Pool", "Spa", "Spillover"})}
			};
		}
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

	namespace
	{
		/// Read a device's label trait, returning std::nullopt (so the caller skips it)
		/// when the device is null or unlabelled.
		std::optional<std::string> DeviceLabel(const std::shared_ptr<Kernel::AuxillaryDevice>& dev)
		{
			if (!dev)
			{
				return std::nullopt;
			}

			auto label = dev->AuxillaryTraits.TryGet(Kernel::AuxillaryTraitsTypes::LabelTrait{});
			if (!label.has_value())
			{
				return std::nullopt;
			}

			return label.value();
		}
	}

	void HomeAssistantDiscovery::AddDynamicDeviceComponents(nlohmann::json& cmps)
	{
		auto data_hub = m_DataHub.lock();
		if (!data_hub)
		{
			return;
		}

		auto add_switch = [&](TopicScheme::DeviceCategory category, const std::shared_ptr<Kernel::AuxillaryDevice>& dev,
			const std::string& state_on, const std::string& state_off)
		{
			auto label = DeviceLabel(dev);
			if (!label.has_value())
			{
				return;
			}

			auto slug = Slugify(label.value());
			auto key = std::format("{}_{}", TopicScheme::CategoryName(category), slug);
			auto state_topic = m_Client->BuildTopic(TopicScheme::DeviceStateSubtopic(category, slug));

			cmps[key] = {
				{"p", "switch"},
				{"name", label.value()},
				{"unique_id", UniqueId(key)},
				{"state_topic", state_topic},
				{"command_topic", DeviceCommandTopic(slug)},
				{"payload_on", "ON"},
				{"payload_off", "OFF"},
				{"state_on", state_on},
				{"state_off", state_off}
			};
		};

		auto add_sensor = [&](TopicScheme::DeviceCategory category, const std::shared_ptr<Kernel::AuxillaryDevice>& dev)
		{
			auto label = DeviceLabel(dev);
			if (!label.has_value())
			{
				return;
			}

			auto slug = Slugify(label.value());
			auto key = std::format("{}_{}", TopicScheme::CategoryName(category), slug);
			auto state_topic = m_Client->BuildTopic(TopicScheme::DeviceStateSubtopic(category, slug));

			cmps[key] = {
				{"p", "sensor"},
				{"name", label.value()},
				{"unique_id", UniqueId(key)},
				{"state_topic", state_topic}
			};
		};

		// Pumps -> switch (Running / Off)
		for (const auto& dev : data_hub->Pumps())
		{
			add_switch(TopicScheme::DeviceCategory::Pump, dev, "Running", "Off");
		}

		// Chlorinators -> switch (On / Off) + extra entities reading the JSON state blob.
		for (const auto& dev : data_hub->Chlorinators())
		{
			add_switch(TopicScheme::DeviceCategory::Chlorinator, dev, "On", "Off");
			AddChlorinatorComponents(cmps, dev);
		}

		// Auxiliaries -> switch (On / Off)
		for (const auto& dev : data_hub->Auxillaries())
		{
			add_switch(TopicScheme::DeviceCategory::Auxillary, dev, "On", "Off");
		}

		// Heaters -> sensor (multi-state: Off/Heating/Enabled)
		for (const auto& dev : data_hub->Heaters())
		{
			add_sensor(TopicScheme::DeviceCategory::Heater, dev);
		}
	}

	void HomeAssistantDiscovery::AddChlorinatorComponents(nlohmann::json& cmps, const std::shared_ptr<Kernel::AuxillaryDevice>& dev)
	{
		auto label = DeviceLabel(dev);
		if (!label.has_value())
		{
			return;
		}

		auto slug = Slugify(label.value());

		// The chlorinator's rich attributes ride the full JSON status blob published by
		// MqttHub to the device topic (TopicScheme::DeviceJsonSubtopic), so these entities
		// point there via value_json templates rather than at the short-string ha/ topic.
		auto state_topic = DeviceStateTopic(slug);

		// Read-only sensors: name suffix + the value_json field they extract.
		struct ChlorinatorSensor
		{
			const char* key_suffix;
			const char* name_suffix;
			const char* value_template;
			const char* unit;        // nullptr => no unit_of_measurement
			bool measurement;        // true => state_class "measurement"
		};

		static constexpr ChlorinatorSensor sensors[] = {
			{ "generating", "Generating %", "{{ value_json.generating_percentage }}", "%", true },
			{ "boost",      "Boost Mode",   "{{ value_json.boost_mode }}",            nullptr, false },
			{ "health",     "Health",       "{{ value_json.chlorinator_health }}",    nullptr, false },
		};

		for (const auto& sensor : sensors)
		{
			auto key = std::format("chlorinator_{}_{}", slug, sensor.key_suffix);

			nlohmann::json component = {
				{"p", "sensor"},
				{"name", std::format("{} {}", label.value(), sensor.name_suffix)},
				{"unique_id", UniqueId(key)},
				{"state_topic", state_topic},
				{"value_template", sensor.value_template}
			};

			if (sensor.unit != nullptr)
			{
				component["unit_of_measurement"] = sensor.unit;
			}

			if (sensor.measurement)
			{
				component["state_class"] = "measurement";
			}

			cmps[key] = std::move(component);
		}

		// Generating-percentage setpoint (number with command topic).
		auto pct_cmd_key = std::format("chlorinator_{}_pct_cmd", slug);
		cmps[pct_cmd_key] = {
			{"p", "number"},
			{"name", std::format("{} Generating Setpoint", label.value())},
			{"unique_id", UniqueId(pct_cmd_key)},
			{"state_topic", state_topic},
			{"value_template", "{{ value_json.generating_percentage }}"},
			{"command_topic", ChlorinatorCommandTopic("percentage")},
			{"min", 0},
			{"max", 100},
			{"step", 1},
			{"unit_of_measurement", "%"},
			{"mode", "slider"}
		};

		// Boost switch (command topic).
		auto boost_cmd_key = std::format("chlorinator_{}_boost_cmd", slug);
		cmps[boost_cmd_key] = {
			{"p", "switch"},
			{"name", std::format("{} Boost", label.value())},
			{"unique_id", UniqueId(boost_cmd_key)},
			{"state_topic", state_topic},
			{"value_template", "{{ value_json.boost_mode }}"},
			{"command_topic", ChlorinatorCommandTopic("boost")},
			{"payload_on", "ON"},
			{"payload_off", "OFF"},
			{"state_on", "Boost"},
			{"state_off", "Off"}
		};
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
			{"sw_version", Version::VersionInfo::ProjectVersionFull()}
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
			{"sw_version", Version::VersionInfo::ProjectVersionFull()},
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
		return Utility::Slugify(input);
	}

	std::string HomeAssistantDiscovery::UniqueId(const std::string& suffix) const
	{
		return std::format("{}_{}", m_Settings.ha_device_id, suffix);
	}

	std::string HomeAssistantDiscovery::AvailabilityTopic() const
	{
		return m_Client->BuildTopic("status/availability");
	}

	std::string HomeAssistantDiscovery::TemperaturesTopic() const
	{
		return m_Client->BuildTopic("pool/temperatures");
	}

	std::string HomeAssistantDiscovery::ChemistryTopic() const
	{
		return m_Client->BuildTopic("pool/chemistry");
	}

	std::string HomeAssistantDiscovery::CirculationTopic() const
	{
		return m_Client->BuildTopic("pool/circulation");
	}

	std::string HomeAssistantDiscovery::ConfigurationTopic() const
	{
		return m_Client->BuildTopic("pool/configuration");
	}

	std::string HomeAssistantDiscovery::SystemStatusTopic() const
	{
		return m_Client->BuildTopic("system/status");
	}

	std::string HomeAssistantDiscovery::AlertStateTopic() const
	{
		return m_Client->BuildTopic(std::string{ Alerting::AlertStateSubtopic });
	}

	void HomeAssistantDiscovery::AddAlertComponents(nlohmann::json& cmps)
	{
		// One HA binary_sensor (device_class: problem) per AlertMonitor condition.
		// All read the same consolidated state document via a per-condition
		// value_template; the AlertMonitor's MQTT sink publishes that document to
		// AlertStateTopic() on every transition.
		const auto state_topic = AlertStateTopic();

		for (const auto& condition : Alerting::AlertConditions)
		{
			const std::string key{ condition.key };

			cmps[std::format("alert_{}", key)] = {
				{ "p", "binary_sensor" },
				{ "name", std::string{ condition.friendly_name } },
				{ "unique_id", UniqueId(std::format("alert_{}", key)) },
				{ "state_topic", state_topic },
				{ "value_template", std::format("{{{{ value_json.{} }}}}", key) },
				{ "device_class", "problem" },
				{ "payload_on", "true" },
				{ "payload_off", "false" }
			};
		}
	}

	std::string HomeAssistantDiscovery::SetpointCommandTopic(const std::string& target) const
	{
		return m_Client->BuildTopic(std::format("command/setpoint/{}", target));
	}

	std::string HomeAssistantDiscovery::DeviceCommandTopic(const std::string& slug) const
	{
		return m_Client->BuildTopic(std::format("command/device/{}", slug));
	}

	std::string HomeAssistantDiscovery::DeviceStateTopic(const std::string& slug) const
	{
		// Routed through TopicScheme so this matches the JSON-blob topic MqttHub publishes.
		return m_Client->BuildTopic(TopicScheme::DeviceJsonSubtopic(slug));
	}

	std::string HomeAssistantDiscovery::ChlorinatorCommandTopic(const std::string& command) const
	{
		return m_Client->BuildTopic(std::format("command/chlorinator/{}", command));
	}

	std::string HomeAssistantDiscovery::CirculationCommandTopic() const
	{
		return m_Client->BuildTopic("command/circulation/mode");
	}

}
// namespace AqualinkAutomate::Mqtt
