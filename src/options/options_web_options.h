#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

namespace AqualinkAutomate::Options::Web
{

	boost::program_options::options_description Options();

	typedef struct
	{
		std::filesystem::path certificate;
		std::filesystem::path private_key;
	}
	SslCertificate;

	typedef struct tagSettings
	{
		tagSettings() :
			http_port{ 80 },
			https_port{ 443 },
			http_content_is_disabled{ false },
			http_server_is_enabled{ true },
			https_server_is_enabled{ true }
		{
		}

		std::string bind_address;
		uint16_t http_port;
		uint16_t https_port;

		std::string doc_root;
		
		bool http_content_is_disabled;
		bool http_server_is_enabled;
		bool https_server_is_enabled;

		SslCertificate ssl_certificate;
		std::optional<std::filesystem::path> ca_chain_certificate;
	}
	Settings;

	Settings HandleOptions(boost::program_options::variables_map& vm);
	void ValidateOptions(boost::program_options::variables_map& vm);

}
// namespace AqualinkAutomate::Options::Web
