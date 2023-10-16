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
	AppOptionPtr OPTION_INTERFACE{ make_appoption("address", "Specific network interface to which to bind", boost::program_options::value<std::string>()->default_value("0.0.0.0")) };
	AppOptionPtr OPTION_PORT{ make_appoption("port", "Specific port number on which to listen", boost::program_options::value<uint16_t>()) };
	AppOptionPtr OPTION_NOCONTENT{ make_appoption("disable-content", "Disable serving of content; only enable HTTP APIs", boost::program_options::bool_switch()->default_value(false)) };
	AppOptionPtr OPTION_NOTLS{ make_appoption("disable-tls", "Disable SSL/TLS support for web services", boost::program_options::bool_switch()->default_value(false)) };
	AppOptionPtr OPTION_TLSCERT{ make_appoption("cert", "Specify the certificate (PEM format) to use", boost::program_options::value<std::string>()->default_value(Application::DEFAULT_CERTIFICATE))};
	AppOptionPtr OPTION_TLSCERTKEY{ make_appoption("cert-key", "Specify the certificate's key (PEM format) to use", boost::program_options::value<std::string>()->default_value(Application::DEFAULT_PRIVATE_KEY)) };
	AppOptionPtr OPTION_TLSCACERT{ make_appoption("cachain-cert", "Specify the CA chain certificate (PEM format) to use", boost::program_options::value<std::string>()) };
	AppOptionPtr OPTION_DOCROOT{ make_appoption("doc-root", "The location from which HTML files are served", boost::program_options::value<std::string>()->default_value(Application::DOC_ROOT)) };

	std::vector WebOptionsCollection
	{
		OPTION_INTERFACE,
		OPTION_PORT,
		OPTION_NOCONTENT,
		OPTION_NOTLS,
		OPTION_TLSCERT,
		OPTION_TLSCERTKEY,
		OPTION_TLSCACERT,
		OPTION_DOCROOT
	};

	boost::program_options::options_description Options()
	{
		const std::string options_collection_name{ "Web" };
		boost::program_options::options_description options(options_collection_name, Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", WebOptionsCollection.size(), options_collection_name));
		for (auto& option : WebOptionsCollection)
		{
			options.add((*option)());
		}

		return options;
	}

	Settings HandleOptions(boost::program_options::variables_map& vm)
	{
		Settings settings;

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

		if (OPTION_NOTLS->IsPresent(vm))
		{ 
			settings.http_server_is_insecure = OPTION_NOTLS->As<bool>(vm);
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
		// The last parameter to set is the web server port (as the defaults are set above then overridden here).
		//

		if (OPTION_PORT->IsPresent(vm))
		{
			settings.bind_port = OPTION_PORT->As<uint16_t>(vm);
		}		
		else
		{
			// Set the default web server port to be the HTTP/HTTPS default.
			settings.bind_port = settings.http_server_is_insecure ? Application::DEFAULT_HTTP_PORT : Application::DEFAULT_HTTPS_PORT;
		}

		return settings;
	}

	void ValidateOptions(boost::program_options::variables_map& vm)
	{
		Helper_ValidateOptionDependencies(vm, OPTION_TLSCERT, OPTION_TLSCERTKEY);
		Helper_ValidateOptionDependencies(vm, OPTION_TLSCERTKEY, OPTION_TLSCERT);
	}

}
// namespace AqualinkAutomate::Options::Web
