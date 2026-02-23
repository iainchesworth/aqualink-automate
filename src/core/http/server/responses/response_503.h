#pragma once

#include <optional>
#include <string_view>

#include "http/server/server_types.h"

namespace AqualinkAutomate::HTTP::Responses
{

	HTTP::Response Response_503(const HTTP::Request& req, std::optional<const std::string_view> reason = std::nullopt);

}
// namespace AqualinkAutomate::HTTP::Responses