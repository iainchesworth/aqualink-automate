#include <cstdint>
#include <format>
#include <string>

#include "logging/logging.h"
#include "options/options_option_type.h"
#include "options/options_web_options.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::Web
{
	AppOptionPtr OPTION_INTERFACE{ make_appoption("address", "Specific network interface to which to bind", boost::program_options::value<std::string>()->default_value("0.0.0.0")) };
	AppOptionPtr OPTION_PORT{ make_appoption("port", "Specific port number on which to listen", boost::program_options::value<uint16_t>()->default_value(80)) };
	AppOptionPtr OPTION_USETLS{ make_appoption("enable-tls", "Enable SSL/TLS support for web services", boost::program_options::bool_switch()->default_value(false)) };
	AppOptionPtr OPTION_TLSCERT{ make_appoption("cert", "Specify the certificate (PEM format) to use", boost::program_options::value<std::string>())};
	AppOptionPtr OPTION_TLSCERTKEY{ make_appoption("cert-key", "Specify the certificate's key (PEM format) to use", boost::program_options::value<std::string>()) };
	AppOptionPtr OPTION_TLSCACERT{ make_appoption("cachain-cert", "Specify the CA chain certificate (PEM format) to use", boost::program_options::value<std::string>()) };
	AppOptionPtr OPTION_TLSCACERTKEY{ make_appoption("cachain-cert-key", "Specify the CA chain certificate's key (PEM format) to use", boost::program_options::value<std::string>()) };
	AppOptionPtr OPTION_DOCROOT{ make_appoption("doc-root", "The location from which HTML files are served", boost::program_options::value<std::string>()->default_value("./templates/")) };

	std::vector WebOptionsCollection
	{
		OPTION_INTERFACE,
		OPTION_PORT,
		OPTION_USETLS,
		OPTION_TLSCERT,
		OPTION_TLSCERTKEY,
		OPTION_TLSCACERT,
		OPTION_TLSCACERTKEY,
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

	Settings HandleOptions(boost::program_options::variables_map vm)
	{
		Settings settings;

		if (OPTION_INTERFACE->IsPresent(vm)) { settings.bind_address = OPTION_INTERFACE->As<std::string>(vm); }
		if (OPTION_PORT->IsPresent(vm)) { settings.bind_port = OPTION_PORT->As<uint16_t>(vm); }

		if (OPTION_USETLS->IsPresent(vm)) { settings.http_server_is_insecure = !(OPTION_USETLS->As<bool>(vm)); }
		if (OPTION_TLSCERT->IsPresent(vm)) { settings.cert_file = OPTION_TLSCERT->As<std::string>(vm); }
		if (OPTION_TLSCERTKEY->IsPresent(vm)) { settings.cert_key_file = OPTION_TLSCERTKEY->As<std::string>(vm); }
		if (OPTION_TLSCACERT->IsPresent(vm)) { settings.ca_chain_cert_file = OPTION_TLSCACERT->As<std::string>(vm); }
		if (OPTION_TLSCACERTKEY->IsPresent(vm)) { settings.ca_chain_cert_key_file = OPTION_TLSCACERTKEY->As<std::string>(vm); }

		if (OPTION_DOCROOT->IsPresent(vm)) { settings.doc_root = OPTION_DOCROOT->As<std::string>(vm); }

		return settings;
	}

}
// namespace AqualinkAutomate::Options::Web
