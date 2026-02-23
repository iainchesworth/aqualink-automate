#include <chrono>
#include <cmath>
#include <format>

#include <boost/uuid/string_generator.hpp>

#include "interfaces/icommanddispatcher.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/circulation.h"
#include "logging/logging.h"
#include "mqtt/mqtt_integration.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Mqtt
{

	namespace
	{
		/// Get current epoch time in seconds for response timestamps.
		int64_t GetTimestampSeconds()
		{
			auto now = std::chrono::system_clock::now();
			return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
		}

		/// Convert CommandResult enum to a string for MQTT responses.
		std::string CommandResultToString(Interfaces::ICommandDispatcher::CommandResult result)
		{
			switch (result)
			{
			case Interfaces::ICommandDispatcher::CommandResult::Success:
				return "success";
			case Interfaces::ICommandDispatcher::CommandResult::DeviceNotFound:
				return "device_not_found";
			case Interfaces::ICommandDispatcher::CommandResult::NoSerialAdapter:
				return "no_serial_adapter";
			case Interfaces::ICommandDispatcher::CommandResult::UnknownEquipmentType:
				return "unknown_equipment_type";
			case Interfaces::ICommandDispatcher::CommandResult::InvalidValue:
				return "invalid_value";
			default:
				return "error";
			}
		}

		/// Build a standard MQTT command response JSON object.
		nlohmann::json BuildCommandResponse(const std::string& command, const std::string& status,
			const nlohmann::json& extra = nlohmann::json::object())
		{
			nlohmann::json response = {
				{"command", command},
				{"status", status},
				{"timestamp", GetTimestampSeconds()}
			};

			for (auto& [key, value] : extra.items())
			{
				response[key] = value;
			}

			return response;
		}

		/// Parse a numeric value from a JSON payload (handles raw string, number, and string types).
		template<typename T>
		T ParsePayloadNumber(const nlohmann::json& payload, T default_value = T{})
		{
			if (payload.contains("raw"))
			{
				return static_cast<T>(std::stoi(payload["raw"].get<std::string>()));
			}
			else if (payload.is_number())
			{
				return payload.get<T>();
			}
			else if (payload.is_string())
			{
				return static_cast<T>(std::stoi(payload.get<std::string>()));
			}
			return default_value;
		}

		/// Parse a string value from a JSON payload (handles raw object and plain string).
		std::string ParsePayloadString(const nlohmann::json& payload)
		{
			if (payload.contains("raw"))
			{
				return payload["raw"].get<std::string>();
			}
			else if (payload.is_string())
			{
				return payload.get<std::string>();
			}
			return {};
		}
	}
	// anonymous namespace

	MqttIntegration::MqttIntegration(boost::asio::io_context& io_context, const Options::Mqtt::MqttSettings& settings)
		: m_Settings(settings)
	{
		if (m_Settings.enabled)
		{
			m_Hub = std::make_shared<MqttHub>(io_context, settings);
			RegisterDefaultCommands();

			if (m_Settings.home_assistant_enabled)
			{
				auto client = m_Hub->GetMqttClient();

				// Configure LWT so the broker publishes "offline" on ungraceful disconnect
				auto availability_topic = client->BuildTopic("status/availability");
				client->SetWill(availability_topic, "offline", /*retain=*/true);

				m_HaDiscovery = std::make_shared<HomeAssistantDiscovery>(client, settings);

				LogInfo(Channel::Mqtt, "MQTT Integration initialized with Home Assistant Discovery (enabled)");
			}
			else
			{
				LogInfo(Channel::Mqtt, "MQTT Integration initialized (enabled)");
			}
		}
		else
		{
			LogInfo(Channel::Mqtt, "MQTT Integration initialized (disabled)");
		}
	}

	void MqttIntegration::Start()
	{
		if (!m_Settings.enabled)
		{
			LogInfo(Channel::Mqtt, "MQTT is disabled, skipping start");
			return;
		}

		if (!m_Hub)
		{
			LogError(Channel::Mqtt, "MQTT Hub not initialized");
			return;
		}

		LogInfo(Channel::Mqtt, "Starting MQTT Integration");

		try
		{
			// Connect HA discovery signals before starting the hub (so we catch the first OnConnected)
			if (m_HaDiscovery)
			{
				auto ha = m_HaDiscovery;

				m_HaConnectedConnection = m_Hub->GetMqttClient()->OnConnected.connect([ha]()
				{
					ha->PublishOnline();
					ha->PublishDiscoveryConfigs();
					ha->PublishDeviceStates();
				});

				m_HaDevicesConnection = m_Hub->OnDevicesPublished.connect([this, ha]()
				{
					ha->PublishDiscoveryConfigs();
					ha->PublishDeviceStates();
					RegisterDynamicDeviceCommands();
				});
			}

			m_Hub->Start();
			LogInfo(Channel::Mqtt, "MQTT Integration started successfully");
		}
		catch (const std::exception& ex)
		{
			LogError(Channel::Mqtt, std::format("Failed to start MQTT Integration: {}", ex.what()));
		}
	}

	void MqttIntegration::Stop()
	{
		if (!m_Settings.enabled || !m_Hub)
		{
			return;
		}

		LogInfo(Channel::Mqtt, "Stopping MQTT Integration");

		try
		{
			m_HaConnectedConnection.disconnect();
			m_HaDevicesConnection.disconnect();

			m_Hub->Stop();
			LogInfo(Channel::Mqtt, "MQTT Integration stopped");
		}
		catch (const std::exception& ex)
		{
			LogError(Channel::Mqtt, std::format("Error stopping MQTT Integration: {}", ex.what()));
		}
	}

	void MqttIntegration::Poll()
	{
		if (m_Hub)
		{
			m_Hub->Poll();
		}
	}

	bool MqttIntegration::IsEnabled() const
	{
		return m_Settings.enabled;
	}

	bool MqttIntegration::IsRunning() const
	{
		return m_Hub && m_Hub->IsRunning();
	}

	void MqttIntegration::ConnectHubs(Kernel::HubLocator& hub_locator)
	{
		try
		{
			auto data_hub = hub_locator.Find<Kernel::DataHub>();
			auto equipment_hub = hub_locator.Find<Kernel::EquipmentHub>();
			auto statistics_hub = hub_locator.Find<Kernel::StatisticsHub>();

			m_CommandDispatcher = hub_locator.TryFind<Interfaces::ICommandDispatcher>();

			ConnectHubs(data_hub, equipment_hub, statistics_hub);
		}
		catch (const std::exception& ex)
		{
			LogWarning(Channel::Mqtt, std::format("Error connecting to hubs via locator: {}", ex.what()));
		}
	}

	void MqttIntegration::ConnectHubs(
		const std::shared_ptr<Kernel::DataHub>& data_hub,
		const std::shared_ptr<Kernel::EquipmentHub>& equipment_hub,
		const std::shared_ptr<Kernel::StatisticsHub>& statistics_hub)
	{
		m_DataHub = data_hub;
		m_EquipmentHub = equipment_hub;
		m_StatisticsHub = statistics_hub;

		if (m_Hub)
		{
			if (data_hub)
			{
				m_Hub->ConnectDataHub(data_hub);
			}

			if (equipment_hub)
			{
				m_Hub->ConnectEquipmentHub(equipment_hub);
			}

			if (statistics_hub)
			{
				m_Hub->ConnectStatisticsHub(statistics_hub);
			}

			if (m_HaDiscovery && data_hub)
			{
				m_HaDiscovery->ConnectDataHub(data_hub);
			}

			// Re-register the device command now that the command dispatcher is available
			RegisterDeviceCommand();
			RegisterSetpointCommand();

			LogDebug(Channel::Mqtt, "MQTT Integration connected to system hubs");
		}
	}

	std::shared_ptr<MqttHub> MqttIntegration::GetMqttHub() const
	{
		return m_Hub;
	}

	void MqttIntegration::RegisterDefaultCommands()
	{
		if (!m_Hub)
		{
			return;
		}

		auto hub = m_Hub;

		// Register status command - republishes all status
		m_Hub->RegisterCommand("status",
			[hub](const std::string& topic, const nlohmann::json& payload)
			{
				LogDebug(Channel::Mqtt, "Received status command");
				try
				{
					if (hub) { hub->PublishAllStatus(); }
				}
				catch (const std::exception& ex)
				{
					LogError(Channel::Mqtt, std::format("Error handling status command: {}", ex.what()));
				}
			});

		// Register device control command (placeholder; upgraded in RegisterDeviceCommand after hubs connect)
		m_Hub->RegisterCommand("device",
			[hub](const std::string& topic, const nlohmann::json& payload)
			{
				LogDebug(Channel::Mqtt, "Received device command");
				try
				{
					std::string device_id = payload.value("device_id", "unknown");
					std::string action = payload.value("action", "toggle");

					auto response = BuildCommandResponse("device", "acknowledged", {{"device_id", device_id}, {"action", action}});
					if (hub) { hub->PublishCustom("response/device", response); }
				}
				catch (const std::exception& ex)
				{
					LogError(Channel::Mqtt, std::format("Error handling device command: {}", ex.what()));
				}
			});

		// Register refresh command - forces status refresh
		m_Hub->RegisterCommand("refresh",
			[hub](const std::string& topic, const nlohmann::json& payload)
			{
				LogDebug(Channel::Mqtt, "Received refresh command");
				try
				{
					if (hub) { hub->PublishAllStatus(); }

					auto response = BuildCommandResponse("refresh", "completed");
					if (hub) { hub->PublishCustom("response/refresh", response); }
				}
				catch (const std::exception& ex)
				{
					LogError(Channel::Mqtt, std::format("Error handling refresh command: {}", ex.what()));
				}
			});

		LogDebug(Channel::Mqtt, "Registered default MQTT command handlers");
	}

	void MqttIntegration::RegisterDeviceCommand()
	{
		if (!m_Hub)
		{
			return;
		}

		auto hub = m_Hub;
		std::weak_ptr<Interfaces::ICommandDispatcher> weak_dispatcher = m_CommandDispatcher;

		m_Hub->RegisterCommand("device",
			[hub, weak_dispatcher](const std::string& topic, const nlohmann::json& payload)
			{
				LogDebug(Channel::Mqtt, "Received device command");
				try
				{
					auto dispatcher = weak_dispatcher.lock();
					if (!dispatcher)
					{
						LogWarning(Channel::Mqtt, "Command dispatcher not available");

						auto response = BuildCommandResponse("device", "error", {{"error", "command dispatcher not available"}});
						if (hub) { hub->PublishCustom("response/device", response); }
						return;
					}

					std::string device_id = payload.value("device_id", "");
					std::string action = payload.value("action", "toggle");

					if (device_id.empty())
					{
						LogWarning(Channel::Mqtt, "Device command missing device_id");
						return;
					}

					Interfaces::ICommandDispatcher::CommandResult result = Interfaces::ICommandDispatcher::CommandResult::DeviceNotFound;

					// Try parsing as UUID first, fall back to label
					try
					{
						boost::uuids::string_generator gen;
						auto uuid = gen(device_id);
						result = dispatcher->ToggleByUuid(uuid);
					}
					catch (const std::runtime_error&)
					{
						// Not a valid UUID, try as label
						result = dispatcher->ToggleByLabel(device_id);
					}

					auto response = BuildCommandResponse("device", CommandResultToString(result),
						{{"device_id", device_id}, {"action", action}});
					if (hub) { hub->PublishCustom("response/device", response); }
				}
				catch (const std::exception& ex)
				{
					LogError(Channel::Mqtt, std::format("Error handling device command: {}", ex.what()));
				}
			});

		LogDebug(Channel::Mqtt, "Registered device command handler with dispatcher");
	}

	void MqttIntegration::RegisterSetpointCommand()
	{
		if (!m_Hub)
		{
			return;
		}

		auto hub = m_Hub;
		std::weak_ptr<Interfaces::ICommandDispatcher> weak_dispatcher = m_CommandDispatcher;
		std::weak_ptr<Kernel::DataHub> weak_data_hub = m_DataHub;

		// Helper lambda to convert Celsius to system unit and dispatch a setpoint command.
		auto dispatch_setpoint = [weak_dispatcher, weak_data_hub, hub](const std::string& target, double celsius_value) -> std::string
		{
			auto dispatcher = weak_dispatcher.lock();
			if (!dispatcher)
			{
				return "error";
			}

			auto data_hub = weak_data_hub.lock();

			// Convert from Celsius to the system's native unit (typically Fahrenheit)
			uint8_t temp_value = 0;
			if (data_hub && data_hub->SystemTemperatureUnits() == Kernel::TemperatureUnits::Celsius)
			{
				temp_value = static_cast<uint8_t>(std::round(celsius_value));
			}
			else
			{
				// Default to Fahrenheit conversion
				temp_value = static_cast<uint8_t>(std::round(celsius_value * 9.0 / 5.0 + 32.0));
			}

			Interfaces::ICommandDispatcher::CommandResult result = Interfaces::ICommandDispatcher::CommandResult::DeviceNotFound;

			if (target == "pool")
			{
				result = dispatcher->SetPoolSetpoint(temp_value);
			}
			else if (target == "spa")
			{
				result = dispatcher->SetSpaSetpoint(temp_value);
			}
			else
			{
				return "invalid_target";
			}

			return CommandResultToString(result);
		};

		// JSON command handler: {"target": "pool"|"spa", "temperature": <celsius>}
		m_Hub->RegisterCommand("setpoint",
			[hub, dispatch_setpoint](const std::string& topic, const nlohmann::json& payload)
			{
				LogDebug(Channel::Mqtt, "Received setpoint command");
				try
				{
					std::string target = payload.value("target", "");
					double temperature = payload.value("temperature", 0.0);

					if (target.empty() || temperature <= 0.0)
					{
						LogWarning(Channel::Mqtt, "Setpoint command missing target or temperature");
						return;
					}

					auto status_str = dispatch_setpoint(target, temperature);

					auto response = BuildCommandResponse("setpoint", status_str,
						{{"target", target}, {"temperature", temperature}});
					if (hub) { hub->PublishCustom("response/setpoint", response); }
				}
				catch (const std::exception& ex)
				{
					LogError(Channel::Mqtt, std::format("Error handling setpoint command: {}", ex.what()));
				}
			});

		// HA number entity handlers: plain text number on setpoint/pool and setpoint/spa
		m_Hub->RegisterCommand("setpoint/pool",
			[hub, dispatch_setpoint](const std::string& topic, const nlohmann::json& payload)
			{
				LogDebug(Channel::Mqtt, "Received HA pool setpoint command");
				try
				{
					auto temperature = ParsePayloadNumber<double>(payload, 0.0);
					if (temperature > 0.0)
					{
						dispatch_setpoint("pool", temperature);
					}
				}
				catch (const std::exception& ex)
				{
					LogError(Channel::Mqtt, std::format("Error handling HA pool setpoint command: {}", ex.what()));
				}
			});

		m_Hub->RegisterCommand("setpoint/spa",
			[hub, dispatch_setpoint](const std::string& topic, const nlohmann::json& payload)
			{
				LogDebug(Channel::Mqtt, "Received HA spa setpoint command");
				try
				{
					auto temperature = ParsePayloadNumber<double>(payload, 0.0);
					if (temperature > 0.0)
					{
						dispatch_setpoint("spa", temperature);
					}
				}
				catch (const std::exception& ex)
				{
					LogError(Channel::Mqtt, std::format("Error handling HA spa setpoint command: {}", ex.what()));
				}
			});

		LogDebug(Channel::Mqtt, "Registered setpoint command handlers");
	}

	void MqttIntegration::RegisterDynamicDeviceCommands()
	{
		if (!m_Hub)
		{
			return;
		}

		auto data_hub = m_DataHub.lock();
		if (!data_hub)
		{
			return;
		}

		std::weak_ptr<Interfaces::ICommandDispatcher> weak_dispatcher = m_CommandDispatcher;
		auto hub = m_Hub;

		auto register_device = [&](const std::shared_ptr<Kernel::AuxillaryDevice>& dev)
		{
			if (!dev)
			{
				return;
			}

			auto label_opt = dev->AuxillaryTraits.TryGet(Kernel::AuxillaryTraitsTypes::LabelTrait{});
			if (!label_opt.has_value())
			{
				return;
			}

			auto label = label_opt.value();
			auto slug = HomeAssistantDiscovery::Slugify(label);
			auto command_key = std::format("device/{}", slug);

			// Skip if already registered
			if (m_Hub->HasCommand(command_key))
			{
				return;
			}

			m_Hub->RegisterCommand(command_key,
				[weak_dispatcher, label, hub](const std::string& topic, const nlohmann::json& payload)
				{
					LogDebug(Channel::Mqtt, std::format("Received device command for '{}'", label));
					try
					{
						auto dispatcher = weak_dispatcher.lock();
						if (!dispatcher)
						{
							LogWarning(Channel::Mqtt, "Command dispatcher not available for device command");
							return;
						}

						// Parse payload: expect "ON" or "OFF" (plain text from HA switch)
						auto action_str = ParsePayloadString(payload);

						Interfaces::ICommandDispatcher::DeviceAction action = Interfaces::ICommandDispatcher::DeviceAction::Toggle;
						if (action_str == "ON")
						{
							action = Interfaces::ICommandDispatcher::DeviceAction::On;
						}
						else if (action_str == "OFF")
						{
							action = Interfaces::ICommandDispatcher::DeviceAction::Off;
						}
						else
						{
							LogWarning(Channel::Mqtt, std::format("Unknown device action payload: '{}'", action_str));
							return;
						}

						auto result = dispatcher->CommandByLabel(label, action);

						LogDebug(Channel::Mqtt, std::format("Device command for '{}': action={}, result={}",
							label, action_str, static_cast<int>(result)));
					}
					catch (const std::exception& ex)
					{
						LogError(Channel::Mqtt, std::format("Error handling device command for '{}': {}", label, ex.what()));
					}
				});
		};

		for (const auto& dev : data_hub->Pumps())
		{
			register_device(dev);
		}

		for (const auto& dev : data_hub->Chlorinators())
		{
			register_device(dev);
		}

		for (const auto& dev : data_hub->Auxillaries())
		{
			register_device(dev);
		}

		// Chlorinator-specific command handlers
		if (!m_Hub->HasCommand("chlorinator/percentage"))
		{
			m_Hub->RegisterCommand("chlorinator/percentage",
				[weak_dispatcher](const std::string& topic, const nlohmann::json& payload)
				{
					LogDebug(Channel::Mqtt, "Received chlorinator percentage command");
					try
					{
						auto dispatcher = weak_dispatcher.lock();
						if (!dispatcher)
						{
							LogWarning(Channel::Mqtt, "Command dispatcher not available for chlorinator percentage");
							return;
						}

						auto percentage = ParsePayloadNumber<uint8_t>(payload);

						auto result = dispatcher->SetChlorinatorPercentage(percentage);
						LogDebug(Channel::Mqtt, std::format("Chlorinator percentage command: {}%, result={}", percentage, static_cast<int>(result)));
					}
					catch (const std::exception& ex)
					{
						LogError(Channel::Mqtt, std::format("Error handling chlorinator percentage command: {}", ex.what()));
					}
				});
		}

		if (!m_Hub->HasCommand("chlorinator/boost"))
		{
			m_Hub->RegisterCommand("chlorinator/boost",
				[weak_dispatcher](const std::string& topic, const nlohmann::json& payload)
				{
					LogDebug(Channel::Mqtt, "Received chlorinator boost command");
					try
					{
						auto dispatcher = weak_dispatcher.lock();
						if (!dispatcher)
						{
							LogWarning(Channel::Mqtt, "Command dispatcher not available for chlorinator boost");
							return;
						}

						auto action_str = ParsePayloadString(payload);
						bool enable = (action_str == "ON");

						auto result = dispatcher->SetChlorinatorBoost(enable);
						LogDebug(Channel::Mqtt, std::format("Chlorinator boost command: {}, result={}", enable ? "ON" : "OFF", static_cast<int>(result)));
					}
					catch (const std::exception& ex)
					{
						LogError(Channel::Mqtt, std::format("Error handling chlorinator boost command: {}", ex.what()));
					}
				});
		}

		// Circulation mode command: accepts "pool", "spa", or "spillover"
		if (!m_Hub->HasCommand("circulation/mode"))
		{
			m_Hub->RegisterCommand("circulation/mode",
				[weak_dispatcher](const std::string& topic, const nlohmann::json& payload)
				{
					LogDebug(Channel::Mqtt, "Received circulation mode command");
					try
					{
						auto dispatcher = weak_dispatcher.lock();
						if (!dispatcher)
						{
							LogWarning(Channel::Mqtt, "Command dispatcher not available for circulation mode");
							return;
						}

						auto mode_str = ParsePayloadString(payload);

						Kernel::CirculationModes mode = Kernel::CirculationModes::Pool;
						if (mode_str == "pool" || mode_str == "Pool")
						{
							mode = Kernel::CirculationModes::Pool;
						}
						else if (mode_str == "spa" || mode_str == "Spa")
						{
							mode = Kernel::CirculationModes::Spa;
						}
						else if (mode_str == "spillover" || mode_str == "Spillover")
						{
							mode = Kernel::CirculationModes::Spillover;
						}
						else
						{
							LogWarning(Channel::Mqtt, std::format("Unknown circulation mode: '{}'", mode_str));
							return;
						}

						auto result = dispatcher->SetCirculationMode(mode);
						LogDebug(Channel::Mqtt, std::format("Circulation mode command: {}, result={}", mode_str, static_cast<int>(result)));
					}
					catch (const std::exception& ex)
					{
						LogError(Channel::Mqtt, std::format("Error handling circulation mode command: {}", ex.what()));
					}
				});
		}
	}

	std::shared_ptr<MqttIntegration> CreateMqttIntegration(boost::asio::io_context& io_context, const Options::Mqtt::MqttSettings& settings)
	{
		if (!settings.enabled)
		{
			LogInfo(Channel::Mqtt, "MQTT is disabled, not creating integration");
			return nullptr;
		}

		return std::make_shared<MqttIntegration>(io_context, settings);
	}

}
// namespace AqualinkAutomate::Mqtt
