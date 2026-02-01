#include <chrono>
#include <format>

#include <magic_enum/magic_enum.hpp>

#include "http/json/json_data_hub.h"
#include "logging/logging.h"
#include "profiling/factories/profiler_factory.h"
#include "profiling/factories/profiling_unit_factory.h"
#include "mqtt/mqtt_hub.h"
#include "utility/latency_percentile_tracker.h"
#include "version/version.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Mqtt
{

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

	void MqttHub::ConnectDataHub(std::shared_ptr<Kernel::DataHub> data_hub)
	{
		m_DataHub = data_hub;

		if (data_hub)
		{
			m_DataHubConnection = data_hub->ConfigUpdateSignal.connect(
				[this](std::shared_ptr<Kernel::DataHub_ConfigEvent> event)
				{
					OnDataHubConfigChanged(event);
				});

			LogInfo(Channel::Mqtt, "MQTT Hub connected to Data Hub");
		}
	}

	void MqttHub::ConnectEquipmentHub(std::shared_ptr<Kernel::EquipmentHub> equipment_hub)
	{
		m_EquipmentHub = equipment_hub;

		if (equipment_hub)
		{
			m_EquipmentHubConnection = equipment_hub->EquipmentStatusChangeSignal.connect(
				[this](std::shared_ptr<Kernel::EquipmentHub_SystemEvent> event)
				{
					OnEquipmentStatusChanged(event);
				});

			LogInfo(Channel::Mqtt, "MQTT Hub connected to Equipment Hub");
		}
	}

	void MqttHub::ConnectStatisticsHub(std::shared_ptr<Kernel::StatisticsHub> statistics_hub)
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

	void MqttHub::PublishAllStatus()
	{
		if (!IsRunning())
		{
			return;
		}

		LogDebug(Channel::Mqtt, "Publishing all status to MQTT");

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

		m_Client->Publish(StatusTopic(subtopic), payload.dump());
	}

	void MqttHub::OnDataHubConfigChanged(std::shared_ptr<Kernel::DataHub_ConfigEvent> event)
	{
		if (!IsRunning() || !event || !m_Settings.publish_on_change)
		{
			return;
		}

		LogTrace(Channel::Mqtt, "Data Hub config changed, publishing update");
	}

	void MqttHub::OnEquipmentStatusChanged(std::shared_ptr<Kernel::EquipmentHub_SystemEvent> event)
	{
		if (!IsRunning() || !event || !m_Settings.publish_on_change)
		{
			return;
		}

		LogTrace(Channel::Mqtt, "Equipment status changed, publishing update");
	}

	void MqttHub::PublishSystemStatus()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MQTT Publish System", std::source_location::current());

		try
		{
			nlohmann::json status = SerializeSystemInfo();
			auto payload = status.dump();
			zone->Value(payload.size());
			m_Client->Publish(StatusTopic("system"), payload);
			LogTrace(Channel::Mqtt, "Published system status");
		}
		catch (const std::exception& ex)
		{
			LogError(Channel::Mqtt, std::format("Failed to publish system status: {}", ex.what()));
		}
	}

	void MqttHub::PublishPoolStatus()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MQTT Publish Pool", std::source_location::current());

		try
		{
			nlohmann::json status = SerializePoolStatus();
			auto payload = status.dump();
			zone->Value(payload.size());
			m_Client->Publish(StatusTopic("pool"), payload);
			LogTrace(Channel::Mqtt, "Published pool status");
		}
		catch (const std::exception& ex)
		{
			LogError(Channel::Mqtt, std::format("Failed to publish pool status: {}", ex.what()));
		}
	}

	void MqttHub::PublishDeviceStatus()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MQTT Publish Devices", std::source_location::current());

		try
		{
			nlohmann::json devices = SerializeDeviceList();
			auto payload = devices.dump();
			zone->Value(payload.size());
			m_Client->Publish(StatusTopic("devices"), payload);
			LogTrace(Channel::Mqtt, "Published device status");
		}
		catch (const std::exception& ex)
		{
			LogError(Channel::Mqtt, std::format("Failed to publish device status: {}", ex.what()));
		}
	}

	void MqttHub::PublishStatistics()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MQTT Publish Stats", std::source_location::current());

		try
		{
			nlohmann::json stats = SerializeStatistics();
			auto payload = stats.dump();
			zone->Value(payload.size());
			m_Client->Publish(StatusTopic("statistics"), payload);
			LogTrace(Channel::Mqtt, "Published statistics");
		}
		catch (const std::exception& ex)
		{
			LogError(Channel::Mqtt, std::format("Failed to publish statistics: {}", ex.what()));
		}
	}

	void MqttHub::HandleMessage(const std::string& topic, const std::string& payload)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MQTT HandleMessage", std::source_location::current());
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
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("MQTT ProcessCommand", std::source_location::current());
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

	nlohmann::json MqttHub::SerializeSystemInfo() const
	{
		auto now = std::chrono::system_clock::now();
		auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::steady_clock::now() - m_StartTime);

		nlohmann::json info = {
			{"timestamp", std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count()},
			{"online", true},
			{"uptime_seconds", uptime.count()},
			{"version", {
				{"application", Version::VersionInfo::ProjectVersion()},
				{"build_date", Version::GitMetadata::CommitDate()}
			}},
			{"mqtt", {
				{"client_id", m_Client ? m_Client->TopicPrefix() : ""},
				{"connected", m_Client ? m_Client->IsConnected() : false}
			}}
		};

		if (auto data_hub = m_DataHub.lock())
		{
			info["equipment"] = {
				{"model_number", data_hub->EquipmentVersions.ModelNumber},
				{"firmware_revision", data_hub->EquipmentVersions.FirmwareRevision}
			};
		}

		return info;
	}

	nlohmann::json MqttHub::SerializePoolStatus() const
	{
		auto now = std::chrono::system_clock::now();

		nlohmann::json status = {
			{"timestamp", std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count()}
		};

		if (auto data_hub = m_DataHub.lock())
		{
			// Temperatures
			status["temperatures"] = {
				{"air", SerializeTemperature(data_hub->AirTemp())},
				{"pool", SerializeTemperature(data_hub->PoolTemp())},
				{"spa", SerializeTemperature(data_hub->SpaTemp())},
				{"freeze_protect", SerializeTemperature(data_hub->FreezeProtectPoint())}
			};

			// Chemistry
			status["chemistry"] = {
				{"orp", {
					{"value_mv", data_hub->ORP()().value()}
				}},
				{"ph", {
					{"value", data_hub->pH()()}
				}},
				{"salt", {
					{"value_ppm", data_hub->SaltLevel().value()}
				}}
			};

			// Circulation mode
			status["circulation"] = {
				{"mode", magic_enum::enum_name(data_hub->CirculationMode)},
				{"spa_mode", data_hub->SpaMode()},
				{"clean_mode", data_hub->InCleanMode}
			};

			// Pool configuration
			status["configuration"] = {
				{"pool_type", magic_enum::enum_name(data_hub->PoolConfiguration)},
				{"system_board", magic_enum::enum_name(data_hub->SystemBoard)}
			};

			// Date/time from controller
			status["controller_time"] = {
				{"date", std::format("{:%Y-%m-%d}", data_hub->Date)},
				{"time", std::format("{:%H:%M:%S}", data_hub->Time)}
			};

			// Timeout remaining
			auto timeout_seconds = data_hub->TimeoutRemaining().count();
			if (timeout_seconds > 0)
			{
				status["timeout_remaining_seconds"] = timeout_seconds;
			}
		}
		else
		{
			status["error"] = "Data hub not available";
		}

		return status;
	}

	nlohmann::json MqttHub::SerializeDeviceList() const
	{
		auto now = std::chrono::system_clock::now();

		nlohmann::json result = {
			{"timestamp", std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count()},
			{"devices", nlohmann::json::object()}
		};

		if (auto data_hub = m_DataHub.lock())
		{
			// Auxiliaries
			nlohmann::json auxillaries = nlohmann::json::array();
			for (const auto& device : data_hub->Auxillaries())
			{
				if (device)
				{
					auxillaries.push_back(SerializeDevice(device));
				}
			}
			result["devices"]["auxillaries"] = auxillaries;

			// Heaters
			nlohmann::json heaters = nlohmann::json::array();
			for (const auto& device : data_hub->Heaters())
			{
				if (device)
				{
					heaters.push_back(SerializeDevice(device));
				}
			}
			result["devices"]["heaters"] = heaters;

			// Pumps
			nlohmann::json pumps = nlohmann::json::array();
			for (const auto& device : data_hub->Pumps())
			{
				if (device)
				{
					pumps.push_back(SerializeDevice(device));
				}
			}
			result["devices"]["pumps"] = pumps;

			// Chlorinators
			nlohmann::json chlorinators = nlohmann::json::array();
			for (const auto& device : data_hub->Chlorinators())
			{
				if (device)
				{
					chlorinators.push_back(SerializeDevice(device));
				}
			}
			result["devices"]["chlorinators"] = chlorinators;

			// Filter pumps
			nlohmann::json filter_pumps = nlohmann::json::array();
			for (const auto& device : data_hub->FilterPumps())
			{
				if (device)
				{
					filter_pumps.push_back(SerializeDevice(device));
				}
			}
			result["devices"]["filter_pumps"] = filter_pumps;
		}
		else
		{
			result["error"] = "Data hub not available";
		}

		return result;
	}

	namespace
	{
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

	nlohmann::json MqttHub::SerializeStatistics() const
	{
		auto now = std::chrono::system_clock::now();

		nlohmann::json stats = {
			{"timestamp", std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count()}
		};

		if (auto statistics_hub = m_StatisticsHub.lock())
		{
			// Message counts
			nlohmann::json message_counts = nlohmann::json::array();
			for (const auto& [msg_id, counter] : statistics_hub->MessageCounts)
			{
				message_counts.push_back({
					{"count", counter.Count()}
				});
			}
			stats["message_counts"] = message_counts;

			// Bandwidth metrics
			stats["bandwidth"] = {
				{"read", {
					{"total_bytes", statistics_hub->BandwidthMetrics.Read.TotalBytes},
					{"utilisation_1sec", statistics_hub->BandwidthMetrics.Read.Average_OneSecond.Utilisation()},
					{"utilisation_30sec", statistics_hub->BandwidthMetrics.Read.Average_ThirtySecond.Utilisation()},
					{"utilisation_5min", statistics_hub->BandwidthMetrics.Read.Average_FiveMinute.Utilisation()}
				}},
				{"write", {
					{"total_bytes", statistics_hub->BandwidthMetrics.Write.TotalBytes},
					{"utilisation_1sec", statistics_hub->BandwidthMetrics.Write.Average_OneSecond.Utilisation()},
					{"utilisation_30sec", statistics_hub->BandwidthMetrics.Write.Average_ThirtySecond.Utilisation()},
					{"utilisation_5min", statistics_hub->BandwidthMetrics.Write.Average_FiveMinute.Utilisation()}
				}}
			};

			// Latency metrics
			stats["latency"] = {
				{"serial_read", SerializeLatencySnapshot(statistics_hub->LatencyMetrics.ReadLatency.GetSnapshot())},
				{"serial_write", SerializeLatencySnapshot(statistics_hub->LatencyMetrics.WriteLatency.GetSnapshot())},
				{"message_processing", SerializeLatencySnapshot(statistics_hub->LatencyMetrics.MessageProcessingLatency.GetSnapshot())},
				{"round_trip", SerializeLatencySnapshot(statistics_hub->LatencyMetrics.RoundTripLatency.GetSnapshot())}
			};

			// Serial metrics
			stats["serial"] = {
				{"message_error_rate", statistics_hub->Serial.MessageErrorRate},
				{"overflow_count", statistics_hub->Serial.SerialOverflowCount},
				{"underflow_count", statistics_hub->Serial.SerialUnderflowCount},
				{"transmission_failures", statistics_hub->Serial.TransmissionFailures},
				{"write_queue_depth", statistics_hub->Serial.SerialWriteQueueDepth}
			};
		}
		else
		{
			stats["error"] = "Statistics hub not available";
		}

		return stats;
	}

	nlohmann::json MqttHub::SerializeTemperature(const Kernel::Temperature& temp) const
	{
		return {
			{"celsius", temp.InCelsius().value()},
			{"fahrenheit", temp.InFahrenheit().value()}
		};
	}

	nlohmann::json MqttHub::SerializeDevice(const std::shared_ptr<Kernel::AuxillaryDevice>& device) const
	{
		if (!device)
		{
			return nullptr;
		}

		nlohmann::json j;
		Kernel::to_json(j, *device);
		return j;
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
