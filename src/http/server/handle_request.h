#pragma once

#include <memory>

#include "http/server/server_types.h"
#include "http/server/router/router.h"

namespace AqualinkAutomate::HTTP
{

	HTTP::Message HandleRequest(std::shared_ptr<HTTP::Router::Router> router, HTTP::Request const& req);

}
// namespace AqualinkAutomate::HTTP
