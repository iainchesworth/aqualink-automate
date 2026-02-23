#pragma once

#include <expected>
#include <string>

#include "http/server/server_types.h"

namespace AqualinkAutomate::HTTP
{

    std::expected<std::string, bool> ParseQueryString(const Request& req, const std::string& query_parameter);

}
// namespace AqualinkAutomate::HTTP
