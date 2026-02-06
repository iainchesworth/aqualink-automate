#include <format>
#include <string>

#include <boost/program_options/value_semantic.hpp>

#include "logging/logging.h"
#include "options/options_mqtt_options.h"
#include "options/helpers/option_dependency_helper.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::Mqtt
{

	boost::program_options::options_description OptionsProcessor::Options() const
	{
		boost::program_options::options_description options(SettingsType::AreaName(), Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", MqttOptionsCollection.size(), SettingsType::AreaName()));
		for (auto& option : MqttOptionsCollection)
		{
			options.add((*option)());
		}

		return options;
	}

	void OptionsProcessor::Validate(const boost::program_options::variables_map& vm) const
	{
		// TLS certificate dependencies
		Helper_ValidateOptionDependencies(vm, OPTION_TLS_CLIENT_CERT, OPTION_TLS_CLIENT_KEY);
		Helper_ValidateOptionDependencies(vm, OPTION_TLS_CLIENT_KEY, OPTION_TLS_CLIENT_CERT);
	}

	std::expected<OptionsProcessor::SettingsType, ErrorCodes::Options_ErrorCodes> OptionsProcessor::Process(boost::program_options::variables_map& vm) const
	{
		SettingsType settings;

		// Enable/disable
		if (OPTION_ENABLE->IsPresent(vm))
		{
			settings.enabled = OPTION_ENABLE->As<bool>(vm);
		}

		// Broker connection
		if (OPTION_BROKER_HOST->IsPresent(vm))
		{
			settings.broker_host = OPTION_BROKER_HOST->As<std::string>(vm);
		}

		if (OPTION_BROKER_PORT->IsPresent(vm))
		{
			settings.broker_port = OPTION_BROKER_PORT->As<uint16_t>(vm);
		}

		if (OPTION_USE_TLS->IsPresent(vm))
		{
			settings.use_tls = OPTION_USE_TLS->As<bool>(vm);

			// If TLS is enabled and port wasn't explicitly set, use default TLS port
			if (settings.use_tls && !OPTION_BROKER_PORT->IsPresentAndNotJustDefaulted(vm))
			{
				settings.broker_port = 8883;
			}
		}

		if (OPTION_TLS_CA_CERT->IsPresent(vm))
		{
			settings.tls_ca_cert = OPTION_TLS_CA_CERT->As<std::string>(vm);
		}

		if (OPTION_TLS_CLIENT_CERT->IsPresent(vm))
		{
			settings.tls_client_cert = OPTION_TLS_CLIENT_CERT->As<std::string>(vm);
		}

		if (OPTION_TLS_CLIENT_KEY->IsPresent(vm))
		{
			settings.tls_client_key = OPTION_TLS_CLIENT_KEY->As<std::string>(vm);
		}

		if (OPTION_TLS_SKIP_VERIFY->IsPresent(vm))
		{
			settings.tls_skip_verify = OPTION_TLS_SKIP_VERIFY->As<bool>(vm);
		}

		// Authentication
		if (OPTION_CLIENT_ID->IsPresent(vm))
		{
			settings.client_id = OPTION_CLIENT_ID->As<std::string>(vm);
		}

		if (OPTION_USERNAME->IsPresent(vm))
		{
			settings.username = OPTION_USERNAME->As<std::string>(vm);
		}

		if (OPTION_PASSWORD->IsPresent(vm))
		{
			settings.password = OPTION_PASSWORD->As<std::string>(vm);
		}

		// Topic configuration
		if (OPTION_TOPIC_PREFIX->IsPresent(vm))
		{
			settings.topic_prefix = OPTION_TOPIC_PREFIX->As<std::string>(vm);
		}

		// Publishing intervals
		if (OPTION_STATUS_INTERVAL->IsPresent(vm))
		{
			settings.status_publish_interval = std::chrono::seconds(OPTION_STATUS_INTERVAL->As<uint32_t>(vm));
		}

		if (OPTION_STATS_INTERVAL->IsPresent(vm))
		{
			settings.statistics_publish_interval = std::chrono::seconds(OPTION_STATS_INTERVAL->As<uint32_t>(vm));
		}

		if (OPTION_PUBLISH_ON_CHANGE->IsPresent(vm))
		{
			settings.publish_on_change = OPTION_PUBLISH_ON_CHANGE->As<bool>(vm);
		}

		LogInfo(Channel::Options, std::format("MQTT settings: enabled={}, broker={}:{}, tls={}",
			settings.enabled, settings.broker_host, settings.broker_port, settings.use_tls));

		return settings;
	}

}
// namespace AqualinkAutomate::Options::Mqtt
