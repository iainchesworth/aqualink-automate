#pragma once

#include <boost/asio/ip/address.hpp>
#include <boost/program_options/options_description.hpp>

namespace AqualinkAutomate::Options::Web
{

	boost::program_options::options_description Options();

	typedef struct
	{
		boost::asio::ip::address address;
		uint16_t port;
	}
	Settings;

	Settings HandleOptions(boost::program_options::variables_map vm);

}
// namespace AqualinkAutomate::Options::Web
