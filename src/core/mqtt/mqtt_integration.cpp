#include <chrono>
#include <format>

#include "logging/logging.h"
#include "mqtt/mqtt_integration.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Mqtt
{

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

				m_HaDevicesConnection = m_Hub->OnDevicesPublished.connect([ha]()
				{
					ha->PublishDiscoveryConfigs();
					ha->PublishDeviceStates();
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

			LogInfo(Channel::Mqtt, "MQTT Integration connected to system hubs");
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
				LogInfo(Channel::Mqtt, "Received status command");
				try
				{
					if (hub) { hub->PublishAllStatus(); }
				}
				catch (const std::exception& ex)
				{
					LogError(Channel::Mqtt, std::format("Error handling status command: {}", ex.what()));
				}
			});

		// Register device control command
		m_Hub->RegisterCommand("device",
			[hub](const std::string& topic, const nlohmann::json& payload)
			{
				LogInfo(Channel::Mqtt, "Received device command");
				try
				{
					if (!payload.contains("device_id"))
					{
						LogWarning(Channel::Mqtt, "Device command missing 'device_id'");
						return;
					}
					if (!payload.contains("action"))
					{
						LogWarning(Channel::Mqtt, "Device command missing 'action'");
						return;
					}

					std::string device_id = payload["device_id"];
					std::string action = payload["action"];
					LogInfo(Channel::Mqtt, std::format("Device command: device={}, action={}", device_id, action));

					auto now = std::chrono::system_clock::now();
					nlohmann::json response = {
						{"command", "device"},
						{"device_id", device_id},
						{"action", action},
						{"status", "acknowledged"},
						{"timestamp", std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count()}
					};

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
				LogInfo(Channel::Mqtt, "Received refresh command");
				try
				{
					if (hub) { hub->PublishAllStatus(); }

					auto now = std::chrono::system_clock::now();
					nlohmann::json response = {
						{"command", "refresh"},
						{"status", "completed"},
						{"timestamp", std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count()}
					};

					if (hub) { hub->PublishCustom("response/refresh", response); }
				}
				catch (const std::exception& ex)
				{
					LogError(Channel::Mqtt, std::format("Error handling refresh command: {}", ex.what()));
				}
			});

		LogDebug(Channel::Mqtt, "Registered default MQTT command handlers");
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
