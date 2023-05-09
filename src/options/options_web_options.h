#pragma once

#include <cstdint>
#include <string>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "certificates/certificate_pemformat.h"

namespace AqualinkAutomate::Options::Web
{

	boost::program_options::options_description Options();

	typedef struct
	{
		std::string bind_address;
		uint16_t bind_port;
		
		bool http_server_is_insecure;

		Certificates::Certificate_PemFormat cert_file;
		Certificates::Certificate_PemFormat cert_key_file;
		Certificates::Certificate_PemFormat ca_chain_cert_file;
		Certificates::Certificate_PemFormat ca_chain_cert_key_file;
	}
	Settings;

	Settings HandleOptions(boost::program_options::variables_map vm);

}
// namespace AqualinkAutomate::Options::Web
