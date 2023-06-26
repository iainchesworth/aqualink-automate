#pragma once

#include <asio/ssl/context.hpp>

#include "options/options_web_options.h"

namespace AqualinkAutomate::Certificates
{

	void LoadWebCertificates(const AqualinkAutomate::Options::Web::Settings& cfg, asio::ssl::context &ctx);

} 
// namespace AqualinkAutomate::Certificates
