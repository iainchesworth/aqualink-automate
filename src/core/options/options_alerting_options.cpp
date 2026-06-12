#include <format>
#include <string>

#include <boost/program_options/value_semantic.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url_view.hpp>

#include "exceptions/exception_optionparsingfailed.h"
#include "logging/logging.h"
#include "options/options_alerting_options.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::Alerting
{

	namespace
	{
		constexpr std::uint32_t SALT_LOW_PPM_MAX{ 6000 };

		// A webhook target must be an absolute http/https URL.
		bool IsValidWebhookUrl(const std::string& url)
		{
			auto parsed = boost::urls::parse_uri(url);
			if (!parsed)
			{
				return false;
			}
			const auto scheme = parsed->scheme();
			return (scheme == "http" || scheme == "https") && parsed->has_authority();
		}
	}
	// unnamed namespace

	boost::program_options::options_description OptionsProcessor::Options() const
	{
		boost::program_options::options_description options(SettingsType::AreaName(), Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", AlertingOptionsCollection.size(), SettingsType::AreaName()));
		for (auto& option : AlertingOptionsCollection)
		{
			options.add((*option)());
		}

		return options;
	}

	void OptionsProcessor::Validate(const boost::program_options::variables_map& vm) const
	{
		// AppOption::IsPresent/As take a non-const variables_map (operator[] is
		// non-const); Validate receives a const ref, so read through a local copy.
		boost::program_options::variables_map vm_local = vm;

		if (OPTION_SALT_LOW_PPM->IsPresent(vm_local))
		{
			const auto ppm = OPTION_SALT_LOW_PPM->As<std::uint32_t>(vm_local);
			if (ppm > SALT_LOW_PPM_MAX)
			{
				throw Exceptions::OptionParsingFailed(std::format("alert-salt-low-ppm must be between 0 and {} (got {})", SALT_LOW_PPM_MAX, ppm));
			}
		}

		if (OPTION_COMMS_TIMEOUT->IsPresent(vm_local))
		{
			const auto secs = OPTION_COMMS_TIMEOUT->As<std::uint32_t>(vm_local);
			if (secs == 0)
			{
				throw Exceptions::OptionParsingFailed("alert-comms-timeout-seconds must be greater than 0");
			}
		}

		if (OPTION_WEBHOOK_URL->IsPresent(vm_local))
		{
			const auto url = OPTION_WEBHOOK_URL->As<std::string>(vm_local);
			if (!IsValidWebhookUrl(url))
			{
				throw Exceptions::OptionParsingFailed("alert-webhook-url must be an absolute http:// or https:// URL");
			}
		}
	}

	std::expected<OptionsProcessor::SettingsType, ErrorCodes::Options_ErrorCodes> OptionsProcessor::Process(boost::program_options::variables_map& vm) const
	{
		SettingsType settings;

		if (OPTION_SALT_LOW_PPM->IsPresent(vm))
		{
			settings.salt_low_ppm = OPTION_SALT_LOW_PPM->As<std::uint32_t>(vm);
		}

		if (OPTION_WEBHOOK_URL->IsPresent(vm))
		{
			settings.webhook_url = OPTION_WEBHOOK_URL->As<std::string>(vm);
		}

		if (OPTION_COMMS_TIMEOUT->IsPresent(vm))
		{
			settings.comms_timeout_seconds = OPTION_COMMS_TIMEOUT->As<std::uint32_t>(vm);
		}

		return settings;
	}

}
// namespace AqualinkAutomate::Options::Alerting
