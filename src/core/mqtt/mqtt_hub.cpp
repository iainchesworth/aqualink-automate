#include <algorithm>
#include <cctype>
#include <chrono>
#include <format>

#include <magic_enum/magic_enum.hpp>

#include "http/json/json_data_hub.h"
#include "logging/logging.h"
#include "mqtt/mqtt_hub.h"
#include "profiling/factories/profiler_factory.h"
#include "profiling/factories/profiling_unit_factory.h"
#include "utility/latency_percentile_tracker.h"
#include "version/version.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Mqtt
{

	namespace
	{
		std::string Slugify(const std::string& input)
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

			if (!result.empty() && result.back() == '_')
			{
				result.pop_back();
			}

			return result;
		}

		double NanosToMicros(std::chrono::nanoseconds ns)
		{
			return static_cast<double>(ns.count()) / 1000.0;
		}

		nlohmann::json SerializeLatencySnapshot(const Utility::LatencyPercentileTracker<>::PercentileSnapshot& snapshot)
		{
			return {
				{"p1_us", NanosToMicros(snapshot.p1)},
				{"p50_us", NanosToMicros(snapshot.p50)},
				{"p95_us", NanosToMicros(snapshot.p95)},
				{"p99_us", NanosToMicros(snapshot.p99)},
				{"min_us", NanosToMicros(snapshot.min)},
				{"max_us", NanosToMicros(snapshot.max)},
				{"mean_us", NanosToMicros(snapshot.mean)},
				{"sample_count", snapshot.sample_count}
			};
		}
	}

	MqttHub::MqttHub(boost::asio::io_context& io_context, const Options::Mqtt::MqttSettings& settings)
		: m_Settings(settings)
		, m_Client(std::make_shared<MqttClient>(io_context, settings))
	{
		LogInfo(Channel::Mqtt, "MQTT Hub initialized");
	}

	MqttHub::~MqttHub()
	{
		m_ClientConnectedConnection.disconnect();
		m_ClientMessageConnection.disconnect();
		m_DataHubConnection.disconnect();
		m_EquipmentHubConnection.disconnect();

		m_Running = false;
	}

	void MqttHub::Start()
	{
		if (m_Running)
		{
			LogWarning(Channel::Mqtt, "MQTT Hub is already running");
			return;
		}

		LogInfo(Channel::Mqtt, "Starting MQTT Hub");

		m_StartTime = std::chrono::steady_clock::now();

		// Connect client signals
		m_ClientConnectedConnection = m_Client->OnConnected.connect([this]()
		{
			Factory::ProfilerFactory::Instance().Get()->Message("MQTT connected to broker");
			LogInfo(Channel::Mqtt, "MQTT Hub: Client connected to broker");

			// Reset static-published flag so version/equipment republish on reconnect
			m_StaticPublished = false;

			// Subscribe to command topics so we receive inbound commands from the broker
			auto command_wildcard = m_Client->BuildTopic("command/#");
			m_Client->Subscribe(command_wildcard, 0);

			// Publish initial status on connect
			PublishAllStatus();
		});

		m_ClientMessageConnection = m_Client->OnMessageReceived.connect(
			[this](const std::string& topic, const std::string& payload)
			{
				HandleMessage(topic, payload);
			});

		// Start the client (sets it to Connecting state)
		m_Client->Start();
		m_Running = true;

		// Initialize periodic publish timers
		auto now = std::chrono::steady_clock::now();
		m_NextStatusPublish = now + m_Settings.status_publish_interval;
		m_NextStatsPublish = now + m_Settings.statistics_publish_interval;

		LogInfo(Channel::Mqtt, "MQTT Hub started successfully");
	}

	void MqttHub::Stop()
	{
		if (!m_Running)
		{
			LogWarning(Channel::Mqtt, "MQTT Hub is not running");
			return;
		}

		LogInfo(Channel::Mqtt, "Stopping MQTT Hub");

		m_Running = false;

		m_ClientConnectedConnection.disconnect();
		m_ClientMessageConnection.disconnect();
		m_DataHubConnection.disconnect();
		m_EquipmentHubConnection.disconnect();

		m_Client->Stop();

		LogInfo(Channel::Mqtt, "MQTT Hub stopped");
	}

	void MqttHub::Poll()
	{
		if (!m_Running)
		{
			return;
		}

		// Advance the MQTT client state machine
		m_Client->Poll();

		// Handle periodic publishing only when connected
		if (!m_Client->IsConnected())
		{
			return;
		}

		auto now = std::chrono::steady_clock::now();

		// Periodic status publish
		if (now >= m_NextStatusPublish)
		{
			PublishSystemStatus();
			PublishPoolStatus();
			PublishDeviceStatus();
			m_NextStatusPublish = now + m_Settings.status_publish_interval;
		}

		// Periodic statistics publish
		if (now >= m_NextStatsPublish)
		{
			PublishStatistics();
			m_NextStatsPublish = now + m_Settings.statistics_publish_interval;
		}
	}

	bool MqttHub::IsRunning() const
	{
		return m_Running && m_Client && m_Client->IsConnected();
	}

	void MqttHub::ConnectDataHub(const std::shared_ptr<Kernel::DataHub>& data_hub)
	{
		m_DataHub = data_hub;

		if (auto locked_hub = m_DataHub.lock())
		{
			m_DataHubConnection = locked_hub->ConfigUpdateSignal.connect(
				[this](const std::shared_ptr<Kernel::DataHub_ConfigEvent>& event)
				{
					OnDataHubConfigChanged(event);
				});

			LogInfo(Channel::Mqtt, "MQTT Hub connected to Data Hub");
		}
	}

	void MqttHub::ConnectEquipmentHub(const std::shared_ptr<Kernel::EquipmentHub>& equipment_hub)
	{
		m_EquipmentHub = equipment_hub;

		if (auto locked_hub = m_EquipmentHub.lock())
		{
			m_EquipmentHubConnection = locked_hub->EquipmentStatusChangeSignal.connect(
				[this](const std::shared_ptr<Kernel::EquipmentHub_SystemEvent>& event)
				{
					OnEquipmentStatusChanged(event);
				});

			LogInfo(Channel::Mqtt, "MQTT Hub connected to Equipment Hub");
		}
	}

	void MqttHub::ConnectStatisticsHub(const std::shared_ptr<Kernel::StatisticsHub>& statistics_hub)
	{
		m_StatisticsHub = statistics_hub;
		LogInfo(Channel::Mqtt, "MQTT Hub connected to Statistics Hub");
	}

	void MqttHub::RegisterCommand(const std::string& command, CommandHandler handler)
	{
		m_CommandHandlers[command] = std::move(handler);
		LogInfo(Channel::Mqtt, std::format("Registered MQTT command handler: {}", command));
	}

	void MqttHub::UnregisterCommand(const std::string& command)
	{
		if (m_CommandHandlers.erase(command) > 0)
		{
			LogInfo(Channel::Mqtt, std::format("Unregistered MQTT command handler: {}", command));
		}
	}

	bool MqttHub::HasCommand(const std::string& command) const
	{
		return m_CommandHandlers.find(command) != m_CommandHandlers.end();
	}

	std::size_t MqttHub::CommandCount() const noexcept
	{
		return m_CommandHandlers.size();
	}

	void MqttHub::PublishAllStatus()
	{
		if (!IsRunning())
		{
			return;
		}

		LogDebug(Channel::Mqtt, "Publishing all status to MQTT");

		PublishStaticTopics();
		PublishSystemStatus();
		PublishPoolStatus();
		PublishDeviceStatus();
		PublishStatistics();
	}

	void MqttHub::PublishCustom(const std::string& subtopic, const nlohmann::json& payload)
	{
		if (!IsRunning())
		{
			return;
		}

		m_Client->Publish(m_Client->BuildTopic(subtopic), payload.dump());
	}

	void MqttHub::OnDataHubConfigChanged(const std::shared_ptr<Kernel::DataHub_ConfigEvent>& event)
	{
		if (!IsRunning() || !event || !m_Settings.publish_on_change)
		{
			return;
		}

		LogTrace(Channel::Mqtt, "Data Hub config changed, publishing update");
	}

	void MqttHub::OnEquipmentStatusChanged(const std::shared_ptr<Kernel::EquipmentHub_SystemEvent>& event)
	{
		if (!IsRunning() || !event || !m_Settings.publish_on_change)
		{
			return;
		}

		LogTrace(Channel::Mqtt, "Equipment status changed, publishing update");
	}

	void MqttHub::PublishStaticTopics()
	{
		if (m_StaticPublished)
		{
			return;
		}

		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MqttHub::PublishStaticTopics", std::source_location::current());

		try
		{
			auto version_payload = SerializeSystemVersion().dump();
			m_Client->Publish(m_Client->BuildTopic("system/version"), version_payload, /*retain=*/true);

			auto equipment_payload = SerializeSystemEquipment().dump();
			m_Client->Publish(m_Client->BuildTopic("system/equipment"), equipment_payload, /*retain=*/true);

			m_StaticPublished = true;
			LogTrace(Channel::Mqtt, "Published static topics (version, equipment)");
		}
		catch (const std::exception& ex)
		{
			LogError(Channel::Mqtt, std::format("Failed to publish static topics: {}", ex.what()));
		}
	}

	void MqttHub::PublishSystemStatus()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MqttHub::PublishSystemStatus", std::source_location::current());

		try
		{
			auto payload = SerializeSystemStatus().dump();
			zone->Value(payload.size());
			m_Client->Publish(m_Client->BuildTopic("system/status"), payload, /*retain=*/true);
			LogTrace(Channel::Mqtt, "Published system status");
		}
		catch (const std::exception& ex)
		{
			LogError(Channel::Mqtt, std::format("Failed to publish system status: {}", ex.what()));
		}
	}

	void MqttHub::PublishPoolStatus()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MqttHub::PublishPoolStatus", std::source_location::current());

		try
		{
			auto temperatures_payload = SerializeTemperatures().dump();
			m_Client->Publish(m_Client->BuildTopic("pool/temperatures"), temperatures_payload, /*retain=*/true);

			auto chemistry_payload = SerializeChemistry().dump();
			m_Client->Publish(m_Client->BuildTopic("pool/chemistry"), chemistry_payload, /*retain=*/true);

			auto circulation_payload = SerializeCirculation().dump();
			m_Client->Publish(m_Client->BuildTopic("pool/circulation"), circulation_payload, /*retain=*/true);

			auto configuration_payload = SerializeConfiguration().dump();
			m_Client->Publish(m_Client->BuildTopic("pool/configuration"), configuration_payload, /*retain=*/true);

			zone->Value(temperatures_payload.size() + chemistry_payload.size() + circulation_payload.size() + configuration_payload.size());
			LogTrace(Channel::Mqtt, "Published pool status (temperatures, chemistry, circulation, configuration)");
		}
		catch (const std::exception& ex)
		{
			LogError(Channel::Mqtt, std::format("Failed to publish pool status: {}", ex.what()));
		}
	}

	void MqttHub::PublishDeviceStatus()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MqttHub::PublishDeviceStatus", std::source_location::current());

		try
		{
			std::size_t total_size = 0;

			if (auto data_hub = m_DataHub.lock())
			{
				auto publish_device = [&](const std::string& type, const std::shared_ptr<Kernel::AuxillaryDevice>& device)
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

					nlohmann::json j;
					Kernel::to_json(j, *device);
					j["type"] = type;

					auto payload = j.dump();
					total_size += payload.size();
					m_Client->Publish(m_Client->BuildTopic(std::format("device/{}", slug)), payload, /*retain=*/true);
				};

				for (const auto& device : data_hub->Auxillaries())
				{
					publish_device("auxillary", device);
				}

				for (const auto& device : data_hub->Heaters())
				{
					publish_device("heater", device);
				}

				for (const auto& device : data_hub->Pumps())
				{
					publish_device("pump", device);
				}

				for (const auto& device : data_hub->Chlorinators())
				{
					publish_device("chlorinator", device);
				}
			}

			zone->Value(total_size);
			LogTrace(Channel::Mqtt, "Published device status");

			OnDevicesPublished();
		}
		catch (const std::exception& ex)
		{
			LogError(Channel::Mqtt, std::format("Failed to publish device status: {}", ex.what()));
		}
	}

	void MqttHub::PublishStatistics()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MqttHub::PublishStatistics", std::source_location::current());

		try
		{
			auto messages_payload = SerializeStatisticsMessages().dump();
			m_Client->Publish(m_Client->BuildTopic("statistics/messages"), messages_payload);

			auto bandwidth_payload = SerializeStatisticsBandwidth().dump();
			m_Client->Publish(m_Client->BuildTopic("statistics/bandwidth"), bandwidth_payload);

			auto latency_payload = SerializeStatisticsLatency().dump();
			m_Client->Publish(m_Client->BuildTopic("statistics/latency"), latency_payload);

			auto serial_payload = SerializeStatisticsSerial().dump();
			m_Client->Publish(m_Client->BuildTopic("statistics/serial"), serial_payload);

			zone->Value(messages_payload.size() + bandwidth_payload.size() + latency_payload.size() + serial_payload.size());
			LogTrace(Channel::Mqtt, "Published statistics");
		}
		catch (const std::exception& ex)
		{
			LogError(Channel::Mqtt, std::format("Failed to publish statistics: {}", ex.what()));
		}
	}

	void MqttHub::HandleMessage(const std::string& topic, const std::string& payload)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MqttHub::HandleMessage", std::source_location::current());
		zone->Text(topic);
		zone->Value(payload.size());

		if (!IsCommandTopic(topic))
		{
			LogTrace(Channel::Mqtt, std::format("Received non-command message on '{}'", topic));
			return;
		}

		std::string command = ExtractCommand(topic);
		LogInfo(Channel::Mqtt, std::format("Received command: {}", command));

		nlohmann::json json_payload;
		if (!payload.empty())
		{
			try
			{
				json_payload = nlohmann::json::parse(payload);
			}
			catch (const nlohmann::json::parse_error& ex)
			{
				LogWarning(Channel::Mqtt, std::format("Failed to parse command payload as JSON: {}", ex.what()));
				json_payload = { {"raw", payload} };
			}
		}

		ProcessCommand(topic, json_payload);
	}

	void MqttHub::ProcessCommand(const std::string& topic, const nlohmann::json& payload)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MqttHub::ProcessCommand", std::source_location::current());
		zone->Text(topic);

		std::string command = ExtractCommand(topic);

		auto it = m_CommandHandlers.find(command);
		if (it != m_CommandHandlers.end())
		{
			LogInfo(Channel::Mqtt, std::format("Executing command: {}", command));
			it->second(topic, payload);
		}
		else
		{
			LogWarning(Channel::Mqtt, std::format("No handler for command: {}", command));
		}
	}

	//=========================================================================
	// Serialization helpers
	//=========================================================================

	nlohmann::json MqttHub::SerializeSystemStatus() const
	{
		auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::steady_clock::now() - m_StartTime);

		return {
			{"online", true},
			{"uptime_seconds", uptime.count()}
		};
	}

	nlohmann::json MqttHub::SerializeSystemVersion() const
	{
		return {
			{"application", Version::VersionInfo::ProjectVersion()},
			{"build_date", Version::GitMetadata::CommitDate()}
		};
	}

	nlohmann::json MqttHub::SerializeSystemEquipment() const
	{
		nlohmann::json equipment;

		if (auto data_hub = m_DataHub.lock())
		{
			equipment["model_number"] = data_hub->EquipmentVersions.ModelNumber();
			equipment["firmware_revision"] = data_hub->EquipmentVersions.FirmwareRevision();
		}

		return equipment;
	}

	nlohmann::json MqttHub::SerializeTemperatures() const
	{
		nlohmann::json temps;

		if (auto data_hub = m_DataHub.lock())
		{
			temps["air"] = SerializeTemperature(data_hub->AirTemp());
			temps["pool"] = SerializeTemperature(data_hub->PoolTemp());
			temps["spa"] = SerializeTemperature(data_hub->SpaTemp());
			temps["freeze_protect"] = SerializeTemperature(data_hub->FreezeProtectPoint());
			temps["pool_setpoint"] = SerializeTemperature(data_hub->PoolTempSetpoint());
			temps["spa_setpoint"] = SerializeTemperature(data_hub->SpaTempSetpoint());
		}

		return temps;
	}

	nlohmann::json MqttHub::SerializeChemistry() const
	{
		nlohmann::json chemistry;

		if (auto data_hub = m_DataHub.lock())
		{
			chemistry["orp"] = {
				{"value_mv", data_hub->ORP()().value()}
			};
			chemistry["ph"] = {
				{"value", data_hub->pH()()}
			};
			chemistry["salt"] = {
				{"value_ppm", data_hub->SaltLevel().value()}
			};
		}

		return chemistry;
	}

	nlohmann::json MqttHub::SerializeCirculation() const
	{
		nlohmann::json circulation;

		if (auto data_hub = m_DataHub.lock())
		{
			circulation["mode"] = magic_enum::enum_name(data_hub->CirculationMode);
			circulation["spa_mode"] = data_hub->SpaMode();
			circulation["clean_mode"] = data_hub->InCleanMode;

			auto timeout_seconds = data_hub->TimeoutRemaining().count();
			if (timeout_seconds > 0)
			{
				circulation["timeout_remaining_seconds"] = timeout_seconds;
			}
		}

		return circulation;
	}

	nlohmann::json MqttHub::SerializeConfiguration() const
	{
		nlohmann::json config;

		if (auto data_hub = m_DataHub.lock())
		{
			config["pool_type"] = magic_enum::enum_name(data_hub->PoolConfiguration);
			config["system_board"] = magic_enum::enum_name(data_hub->SystemBoard);
			config["date"] = std::format("{:%Y-%m-%d}", data_hub->Date);
			config["time"] = std::format("{:%H:%M:%S}", data_hub->Time);
		}

		return config;
	}

	nlohmann::json MqttHub::SerializeStatisticsMessages() const
	{
		nlohmann::json messages = nlohmann::json::array();

		if (auto statistics_hub = m_StatisticsHub.lock())
		{
			for (const auto& [msg_id, counter] : statistics_hub->MessageCounts)
			{
				messages.push_back({
					{"count", counter.Count()}
				});
			}
		}

		return messages;
	}

	nlohmann::json MqttHub::SerializeStatisticsBandwidth() const
	{
		nlohmann::json bandwidth;

		if (auto statistics_hub = m_StatisticsHub.lock())
		{
			bandwidth["read"] = {
				{"total_bytes", statistics_hub->BandwidthMetrics.Read.TotalBytes},
				{"utilisation_1sec", statistics_hub->BandwidthMetrics.Read.Average_OneSecond.Utilisation()},
				{"utilisation_30sec", statistics_hub->BandwidthMetrics.Read.Average_ThirtySecond.Utilisation()},
				{"utilisation_5min", statistics_hub->BandwidthMetrics.Read.Average_FiveMinute.Utilisation()}
			};
			bandwidth["write"] = {
				{"total_bytes", statistics_hub->BandwidthMetrics.Write.TotalBytes},
				{"utilisation_1sec", statistics_hub->BandwidthMetrics.Write.Average_OneSecond.Utilisation()},
				{"utilisation_30sec", statistics_hub->BandwidthMetrics.Write.Average_ThirtySecond.Utilisation()},
				{"utilisation_5min", statistics_hub->BandwidthMetrics.Write.Average_FiveMinute.Utilisation()}
			};
		}

		return bandwidth;
	}

	nlohmann::json MqttHub::SerializeStatisticsLatency() const
	{
		nlohmann::json latency;

		if (auto statistics_hub = m_StatisticsHub.lock())
		{
			latency["serial_read"] = SerializeLatencySnapshot(statistics_hub->LatencyMetrics.ReadLatency.GetSnapshot());
			latency["serial_write"] = SerializeLatencySnapshot(statistics_hub->LatencyMetrics.WriteLatency.GetSnapshot());
			latency["message_processing"] = SerializeLatencySnapshot(statistics_hub->LatencyMetrics.MessageProcessingLatency.GetSnapshot());
		}

		return latency;
	}

	nlohmann::json MqttHub::SerializeStatisticsSerial() const
	{
		nlohmann::json serial;

		if (auto statistics_hub = m_StatisticsHub.lock())
		{
			serial["message_error_rate"] = statistics_hub->Serial.MessageErrorRate;
			serial["overflow_count"] = statistics_hub->Serial.SerialOverflowCount;
			serial["underflow_count"] = statistics_hub->Serial.SerialUnderflowCount;
			serial["transmission_failures"] = statistics_hub->Serial.TransmissionFailures;
			serial["write_queue_depth"] = statistics_hub->Serial.SerialWriteQueueDepth;
		}

		return serial;
	}

	nlohmann::json MqttHub::SerializeTemperature(const Kernel::Temperature& temp) const
	{
		return {
			{"celsius", temp.InCelsius().value()},
			{"fahrenheit", temp.InFahrenheit().value()}
		};
	}

	std::string MqttHub::StatusTopic(const std::string& subtopic) const
	{
		return m_Client->BuildTopic(std::format("{}/{}", STATUS_PREFIX, subtopic));
	}

	std::string MqttHub::EventTopic(const std::string& event_type) const
	{
		return m_Client->BuildTopic(std::format("{}/{}", EVENT_PREFIX, event_type));
	}

	std::string MqttHub::CommandTopic(const std::string& action) const
	{
		return m_Client->BuildTopic(std::format("{}/{}", COMMAND_PREFIX, action));
	}

	bool MqttHub::IsCommandTopic(const std::string& topic) const
	{
		std::string prefix = m_Client->BuildTopic(COMMAND_PREFIX);
		return topic.find(prefix) == 0;
	}

	std::string MqttHub::ExtractCommand(const std::string& topic) const
	{
		std::string prefix = m_Client->BuildTopic(std::format("{}/", COMMAND_PREFIX));

		if (topic.find(prefix) == 0)
		{
			return topic.substr(prefix.length());
		}

		return "";
	}

}
// namespace AqualinkAutomate::Mqtt
