#include <cstdint>
#include <format>
#include <string>

#include "application/application_defaults.h"
#include "logging/logging.h"
#include "options/options_option_type.h"
#include "options/options_web_options.h"
#include "options/helpers/conflicting_options_helper.h"
#include "options/helpers/option_dependency_helper.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::Web
{
	
	boost::program_options::options_description OptionsProcessor::Options() const
	{
		boost::program_options::options_description options(SettingsType::AreaName(), Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", WebOptionsCollection.size(), SettingsType::AreaName()));
		for (auto& option : WebOptionsCollection)
		{
			options.add((*option)());
		}

		return options;
	}

	void OptionsProcessor::Validate(const boost::program_options::variables_map& vm) const
	{
		Helper_ValidateOptionDependencies(vm, OPTION_TLSCERT, OPTION_TLSCERTKEY);
		Helper_ValidateOptionDependencies(vm, OPTION_TLSCERTKEY, OPTION_TLSCERT);

		Helper_CheckForConflictingOptions(vm, OPTION_NOHTTP, OPTION_HTTPPORT);
		Helper_CheckForConflictingOptions(vm, OPTION_NOHTTPS, OPTION_HTTPSPORT);
		Helper_CheckForConflictingOptions(vm, OPTION_NOHTTPS, OPTION_TLSCERT);
		Helper_CheckForConflictingOptions(vm, OPTION_NOHTTPS, OPTION_TLSCERTKEY);
		Helper_CheckForConflictingOptions(vm, OPTION_NOHTTPS, OPTION_TLSCACERT);
	}

	std::expected<OptionsProcessor::SettingsType, ErrorCodes::Options_ErrorCodes> OptionsProcessor::Process(boost::program_options::variables_map& vm) const
	{
		SettingsType settings;

		if (OPTION_INTERFACE->IsPresent(vm))
		{
			settings.bind_address = OPTION_INTERFACE->As<std::string>(vm);
		}

		if (OPTION_NOCONTENT->IsPresent(vm))
		{
			settings.http_content_is_disabled = OPTION_NOCONTENT->As<bool>(vm);
		}

		if (OPTION_DOCROOT->IsPresent(vm))
		{
			settings.doc_root = OPTION_DOCROOT->As<std::string>(vm);
		}

		if (OPTION_NOHTTP->IsPresent(vm))
		{
			// Note the inverted setting vs. parameter
			settings.http_server_is_enabled = !(OPTION_NOHTTP->As<bool>(vm));
		}

		if (OPTION_NOHTTPS->IsPresent(vm))
		{
			// Note the inverted setting vs. parameter
			settings.https_server_is_enabled = !(OPTION_NOHTTPS->As<bool>(vm));
		}

		if (OPTION_TLSCERT->IsPresent(vm) && OPTION_TLSCERTKEY->IsPresent(vm))
		{
			settings.ssl_certificate = SslCertificate
			{
				std::filesystem::path(OPTION_TLSCERT->As<std::string>(vm)),
				std::filesystem::path(OPTION_TLSCERTKEY->As<std::string>(vm))
			};
		}

		if (OPTION_TLSCACERT->IsPresent(vm))
		{
			settings.ca_chain_certificate = std::filesystem::path(OPTION_TLSCACERT->As<std::string>(vm));
		}

		//
		// The last parameter to set is the web server ports (as the defaults are set above then overridden here).
		//

		if (OPTION_HTTPPORT->IsPresent(vm))
		{
			settings.http_port = OPTION_HTTPPORT->As<uint16_t>(vm);
		}
		else
		{
			// Set the default web server port to be the HTTP default.
			settings.http_port = Application::DEFAULT_HTTP_PORT;
		}

		if (OPTION_HTTPSPORT->IsPresent(vm))
		{
			settings.https_port = OPTION_HTTPSPORT->As<uint16_t>(vm);
		}
		else
		{
			// Set the default web server port to be the HTTPS default.
			settings.https_port = Application::DEFAULT_HTTPS_PORT;
		}

		return settings;
	}

}
// namespace AqualinkAutomate::Options::Web
