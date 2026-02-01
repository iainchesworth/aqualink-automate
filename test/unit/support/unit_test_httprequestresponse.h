#pragma once

#include <string_view>

#include "http/server/server_types.h"

namespace AqualinkAutomate::Test
{

	HTTP::Response PerformHttpRequestResponse(const std::string_view& url_to_retrieve);

}
// namespace AqualinkAutomate::Test
