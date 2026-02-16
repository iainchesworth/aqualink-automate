#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include <boost/asio/io_context.hpp>
#include <boost/signals2.hpp>
#include <nlohmann/json.hpp>

#include "interfaces/ihub.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/statistics_hub.h"
#include "mqtt/mqtt_client.h"
#include "options/options_mqtt_options.h"


namespace AqualinkAutomate::Mqtt
{

	/// MQTT Hub integrates pool controller data from the kernel hubs and publishes
	/// it to MQTT topics. It also handles incoming command messages.
	///
	/// Topic structure:
	/// - {prefix}/status/availability     - Online/offline status (LWT, retained)
	/// - {prefix}/system/status           - System online + uptime (retained)
	/// - {prefix}/system/version          - Application version info (retained, static)
	/// - {prefix}/system/equipment        - Equipment model/firmware (retained, static)
	/// - {prefix}/pool/temperatures       - Temperature readings and setpoints (retained)
	/// - {prefix}/pool/chemistry          - ORP, pH, salt (retained)
	/// - {prefix}/pool/circulation        - Circulation mode, spa mode (retained)
	/// - {prefix}/pool/configuration      - Pool type, system board, date/time (retained)
	/// - {prefix}/device/{slug}           - Per-device state (retained)
	/// - {prefix}/statistics/messages     - Message counts (not retained)
	/// - {prefix}/statistics/bandwidth    - Bandwidth metrics (not retained)
	/// - {prefix}/statistics/latency      - Latency percentiles (not retained)
	/// - {prefix}/statistics/serial       - Serial error metrics (not retained)
	/// - {prefix}/event/{type}            - Real-time events
	/// - {prefix}/command/{action}        - Command topics for control
	class MqttHub : public std::enable_shared_from_this<MqttHub>
	{
	public:
		using CommandHandler = std::function<void(const std::string& topic, const nlohmann::json& payload)>;

	public:
		explicit MqttHub(boost::asio::io_context& io_context, const Options::Mqtt::MqttSettings& settings);
		~MqttHub();

		// Non-copyable, non-movable
		MqttHub(const MqttHub&) = delete;
		MqttHub& operator=(const MqttHub&) = delete;
		MqttHub(MqttHub&&) = delete;
		MqttHub& operator=(MqttHub&&) = delete;

	public:
		//---------------------------------------------------------------------
		// LIFECYCLE
		//---------------------------------------------------------------------

		/// Start the MQTT hub and begin publishing data.
		void Start();

		/// Stop the MQTT hub.
		void Stop();

		/// Poll the MQTT hub (advances client state machine and periodic publishing).
		void Poll();

		/// Check if the hub is running and connected.
		bool IsRunning() const;

		//---------------------------------------------------------------------
		// HUB INTEGRATION
		//---------------------------------------------------------------------

		/// Connect to the data hub to receive pool configuration data.
		void ConnectDataHub(const std::shared_ptr<Kernel::DataHub>& data_hub);

		/// Connect to the equipment hub to receive equipment status updates.
		void ConnectEquipmentHub(const std::shared_ptr<Kernel::EquipmentHub>& equipment_hub);

		/// Connect to the statistics hub to receive message and bandwidth stats.
		void ConnectStatisticsHub(const std::shared_ptr<Kernel::StatisticsHub>& statistics_hub);

		//---------------------------------------------------------------------
		// COMMAND HANDLING
		//---------------------------------------------------------------------

		/// Register a handler for a specific command.
		void RegisterCommand(const std::string& command, CommandHandler handler);

		/// Unregister a command handler.
		void UnregisterCommand(const std::string& command);

		/// Check if a handler is registered for a specific command.
		bool HasCommand(const std::string& command) const;

		/// Get the number of registered command handlers.
		std::size_t CommandCount() const noexcept;

		//---------------------------------------------------------------------
		// MANUAL PUBLISHING
		//---------------------------------------------------------------------

		/// Publish all current status immediately.
		void PublishAllStatus();

		/// Publish a custom JSON payload to a subtopic.
		void PublishCustom(const std::string& subtopic, const nlohmann::json& payload);

		/// Get the underlying MQTT client (for use by HA discovery and similar layers).
		std::shared_ptr<MqttClient> GetMqttClient() const { return m_Client; }

		/// Signal fired after device status is published. Used by HA discovery to update device states.
		boost::signals2::signal<void()> OnDevicesPublished;

		//---------------------------------------------------------------------
		// TOPIC UTILITIES
		//---------------------------------------------------------------------

		std::string StatusTopic(const std::string& subtopic) const;
		std::string EventTopic(const std::string& event_type) const;
		std::string CommandTopic(const std::string& action) const;
		bool IsCommandTopic(const std::string& topic) const;
		std::string ExtractCommand(const std::string& topic) const;

	private:
		//---------------------------------------------------------------------
		// SIGNAL HANDLERS
		//---------------------------------------------------------------------

		void OnDataHubConfigChanged(const std::shared_ptr<Kernel::DataHub_ConfigEvent>& event);
		void OnEquipmentStatusChanged(const std::shared_ptr<Kernel::EquipmentHub_SystemEvent>& event);

		//---------------------------------------------------------------------
		// PUBLISHING METHODS
		//---------------------------------------------------------------------

		void PublishStaticTopics();
		void PublishSystemStatus();
		void PublishPoolStatus();
		void PublishDeviceStatus();
		void PublishStatistics();

		//---------------------------------------------------------------------
		// MESSAGE HANDLING
		//---------------------------------------------------------------------

		void HandleMessage(const std::string& topic, const std::string& payload);
		void ProcessCommand(const std::string& topic, const nlohmann::json& payload);

		//---------------------------------------------------------------------
		// SERIALIZATION HELPERS
		//---------------------------------------------------------------------

		nlohmann::json SerializeSystemStatus() const;
		nlohmann::json SerializeSystemVersion() const;
		nlohmann::json SerializeSystemEquipment() const;
		nlohmann::json SerializeTemperatures() const;
		nlohmann::json SerializeChemistry() const;
		nlohmann::json SerializeCirculation() const;
		nlohmann::json SerializeConfiguration() const;
		nlohmann::json SerializeStatisticsMessages() const;
		nlohmann::json SerializeStatisticsBandwidth() const;
		nlohmann::json SerializeStatisticsLatency() const;
		nlohmann::json SerializeStatisticsSerial() const;
		nlohmann::json SerializeTemperature(const Kernel::Temperature& temp) const;

	private:
		const Options::Mqtt::MqttSettings m_Settings;
		std::shared_ptr<MqttClient> m_Client;

		// Hub connections
		std::weak_ptr<Kernel::DataHub> m_DataHub;
		std::weak_ptr<Kernel::EquipmentHub> m_EquipmentHub;
		std::weak_ptr<Kernel::StatisticsHub> m_StatisticsHub;

		// Signal connections
		boost::signals2::scoped_connection m_DataHubConnection;
		boost::signals2::scoped_connection m_EquipmentHubConnection;
		boost::signals2::scoped_connection m_ClientConnectedConnection;
		boost::signals2::scoped_connection m_ClientMessageConnection;

		// Command handlers
		std::unordered_map<std::string, CommandHandler> m_CommandHandlers;

		// State
		bool m_Running{ false };
		bool m_StaticPublished{ false };
		std::chrono::steady_clock::time_point m_StartTime;

		// Periodic publish timers (using steady_clock comparisons)
		std::chrono::steady_clock::time_point m_NextStatusPublish;
		std::chrono::steady_clock::time_point m_NextStatsPublish;

		// Topic prefixes
		static constexpr const char* STATUS_PREFIX = "status";
		static constexpr const char* EVENT_PREFIX = "event";
		static constexpr const char* COMMAND_PREFIX = "command";
	};

}
// namespace AqualinkAutomate::Mqtt
