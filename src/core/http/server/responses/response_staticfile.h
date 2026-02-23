#pragma once

#include <filesystem>

#include "http/server/server_types.h"

namespace AqualinkAutomate::HTTP::Responses
{

	HTTP::Message Response_StaticFile(const HTTP::Request& req, const std::filesystem::path& result);

}
// namespace AqualinkAutomate::HTTP::Responses