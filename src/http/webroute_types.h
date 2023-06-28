#pragma once

#include <functional>

#include <crow/app.h>

namespace AqualinkAutomate::HTTP
{

	using Request = crow::request;
	using Response = crow::response;
	using RouteParams = crow::routing_params;
	using RouteHandler = std::function<void(const Request&, Response&)>;

}
// namespace AqualinkAutomate::HTTP
