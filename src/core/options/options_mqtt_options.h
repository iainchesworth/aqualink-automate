#pragma once

#include <chrono>
#include <cstdint>
#include <expected>
#include <string>
#include <vector>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "errors/options_errors.h"
#include "options/options_option_type.h"

namespace AqualinkAutomate::Options::Mqtt
{

	/// Quality of Service levels for MQTT message delivery
	enum class QoS : uint8_t
	{
		AtMostOnce = 0,   // Fire and forget - no acknowledgment
		AtLeastOnce = 1,  // Acknowledged delivery - may be delivered more than once
		ExactlyOnce = 2   // Assured delivery - exactly once
	};

	/// MQTT connection and publishing settings
	typedef struct tagMqttSettings
	{
		static const std::string& AreaName()
		{
			static const std::string AREA_NAME{ "MQTT" };
			return AREA_NAME;
		}

		tagMqttSettings() = default;

		//---------------------------------------------------------------------
		// ENABLE/DISABLE
		//---------------------------------------------------------------------

		/// Enable or disable MQTT functionality
		bool enabled{ false };

		//---------------------------------------------------------------------
		// BROKER CONNECTION
		//---------------------------------------------------------------------

		/// MQTT broker hostname or IP address
		std::string broker_host{ "localhost" };

		/// MQTT broker port (default: 1883 for TCP, 8883 for TLS)
		uint16_t broker_port{ 1883 };

		/// Enable TLS/SSL encryption
		bool use_tls{ false };

		/// TLS CA certificate file path (for server verification)
		std::string tls_ca_cert;

		/// TLS client certificate file path (for client authentication)
		std::string tls_client_cert;

		/// TLS client key file path
		std::string tls_client_key;

		/// Skip TLS certificate verification (not recommended for production)
		bool tls_skip_verify{ false };

		//---------------------------------------------------------------------
		// AUTHENTICATION
		//---------------------------------------------------------------------

		/// Client identifier (auto-generated if empty)
		std::string client_id;

		/// Username for broker authentication (optional)
		std::string username;

		/// Password for broker authentication (optional)
		std::string password;

		//---------------------------------------------------------------------
		// TOPIC CONFIGURATION
		//---------------------------------------------------------------------

		/// Base topic prefix for all published messages
		/// Example: "aqualink" -> "aqualink/status/temperature"
		std::string topic_prefix{ "aqualink" };

		//---------------------------------------------------------------------
		// RECONNECTION
		//---------------------------------------------------------------------

		/// Initial reconnection delay
		std::chrono::seconds reconnect_delay_initial{ 5 };

		/// Maximum reconnection delay (exponential backoff)
		std::chrono::seconds reconnect_delay_max{ 300 };

		//---------------------------------------------------------------------
		// PUBLISHING INTERVALS
		//---------------------------------------------------------------------

		/// Interval for periodic status publishing
		std::chrono::seconds status_publish_interval{ 30 };

		/// Interval for statistics publishing
		std::chrono::seconds statistics_publish_interval{ 60 };

		/// Publish on change (in addition to periodic)
		bool publish_on_change{ true };

	}
	MqttSettings;

	class OptionsProcessor
	{
	private:
		// Enable/disable
		AppOptionPtr OPTION_ENABLE{ make_appoption("mqtt", "Enable MQTT client for publishing pool data", boost::program_options::bool_switch()->default_value(false)) };

		// Broker connection
		AppOptionPtr OPTION_BROKER_HOST{ make_appoption("mqtt-host", "MQTT broker hostname or IP address", boost::program_options::value<std::string>()->default_value("localhost")) };
		AppOptionPtr OPTION_BROKER_PORT{ make_appoption("mqtt-port", "MQTT broker port", boost::program_options::value<uint16_t>()->default_value(1883)) };
		AppOptionPtr OPTION_USE_TLS{ make_appoption("mqtt-tls", "Enable TLS/SSL for MQTT connection", boost::program_options::bool_switch()->default_value(false)) };
		AppOptionPtr OPTION_TLS_CA_CERT{ make_appoption("mqtt-ca-cert", "TLS CA certificate file path", boost::program_options::value<std::string>()) };
		AppOptionPtr OPTION_TLS_CLIENT_CERT{ make_appoption("mqtt-client-cert", "TLS client certificate file path", boost::program_options::value<std::string>()) };
		AppOptionPtr OPTION_TLS_CLIENT_KEY{ make_appoption("mqtt-client-key", "TLS client key file path", boost::program_options::value<std::string>()) };
		AppOptionPtr OPTION_TLS_SKIP_VERIFY{ make_appoption("mqtt-tls-skip-verify", "Skip TLS certificate verification", boost::program_options::bool_switch()->default_value(false)) };

		// Authentication
		AppOptionPtr OPTION_CLIENT_ID{ make_appoption("mqtt-client-id", "MQTT client identifier (auto-generated if not specified)", boost::program_options::value<std::string>()) };
		AppOptionPtr OPTION_USERNAME{ make_appoption("mqtt-username", "Username for MQTT broker authentication", boost::program_options::value<std::string>()) };
		AppOptionPtr OPTION_PASSWORD{ make_appoption("mqtt-password", "Password for MQTT broker authentication", boost::program_options::value<std::string>()) };

		// Topic configuration
		AppOptionPtr OPTION_TOPIC_PREFIX{ make_appoption("mqtt-prefix", "Base topic prefix for all MQTT messages", boost::program_options::value<std::string>()->default_value("aqualink")) };

		// Publishing intervals
		AppOptionPtr OPTION_STATUS_INTERVAL{ make_appoption("mqtt-status-interval", "Status publish interval in seconds", boost::program_options::value<uint32_t>()->default_value(30)) };
		AppOptionPtr OPTION_STATS_INTERVAL{ make_appoption("mqtt-stats-interval", "Statistics publish interval in seconds", boost::program_options::value<uint32_t>()->default_value(60)) };
		AppOptionPtr OPTION_PUBLISH_ON_CHANGE{ make_appoption("mqtt-on-change", "Publish immediately on data changes", boost::program_options::bool_switch()->default_value(true)) };

		std::vector<AppOptionPtr> MqttOptionsCollection
		{
			OPTION_ENABLE,
			OPTION_BROKER_HOST,
			OPTION_BROKER_PORT,
			OPTION_USE_TLS,
			OPTION_TLS_CA_CERT,
			OPTION_TLS_CLIENT_CERT,
			OPTION_TLS_CLIENT_KEY,
			OPTION_TLS_SKIP_VERIFY,
			OPTION_CLIENT_ID,
			OPTION_USERNAME,
			OPTION_PASSWORD,
			OPTION_TOPIC_PREFIX,
			OPTION_STATUS_INTERVAL,
			OPTION_STATS_INTERVAL,
			OPTION_PUBLISH_ON_CHANGE
		};

	public:
		using SettingsType = MqttSettings;

	public:
		std::string Name() const { return SettingsType::AreaName(); }
		boost::program_options::options_description Options() const;

	public:
		void Validate(const boost::program_options::variables_map& vm) const;
		std::expected<SettingsType, ErrorCodes::Options_ErrorCodes> Process(boost::program_options::variables_map& vm) const;
	};

}
// namespace AqualinkAutomate::Options::Mqtt
