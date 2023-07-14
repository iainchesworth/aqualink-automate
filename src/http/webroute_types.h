#pragma once

#include <functional>
#include <memory>

#include <cinatra.hpp>

namespace AqualinkAutomate::HTTP
{

	using Connection = std::shared_ptr<cinatra::connection<cinatra::NonSSL>>;
	using Server = cinatra::http_server;
	using Methods = cinatra::http_method;
	using Request = cinatra::request;
	using Response = cinatra::response;
	using RouteHandler = std::function<void(Request&, Response&)>;

}
// namespace AqualinkAutomate::HTTP
