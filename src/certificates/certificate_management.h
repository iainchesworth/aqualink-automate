#pragma once

#include <boost/asio/ssl/context.hpp>

#include "options/options_web_options.h"

namespace AqualinkAutomate::Certificates
{

	void LoadSslCertificates(const AqualinkAutomate::Options::Web::Settings& cfg, boost::asio::ssl::context &ctx);

} 
// namespace AqualinkAutomate::Certificates
