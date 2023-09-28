#pragma once

#include <string>

#include <tl/expected.hpp>

#include "http/server/server_types.h"

namespace AqualinkAutomate::HTTP
{

    tl::expected<std::string, bool> ParseQueryString(const Request& req, const std::string& query_parameter);

}
// namespace AqualinkAutomate::HTTP
