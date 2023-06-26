#pragma once

#include <functional>
#include <string>

#include <crow/app.h>
#include <crow/mustache.h>
#include <crow/routing.h>

#include "concepts/is_c_array.h"

namespace AqualinkAutomate::Interfaces
{

	template<const auto& ROUTE_URL>
	requires (Concepts::CArrayRef<decltype(ROUTE_URL)>)
	class IWebRoute
	{
	public:
		using Request = crow::request;
		using Response = crow::response;
		using RouteParams = crow::routing_params;

	public:
		explicit IWebRoute(crow::SimpleApp& app)
		{
			// CROW_ROUTE(app, ROUTE_URL)
			app.template route<crow::black_magic::get_parameter_tag(ROUTE_URL)>(ROUTE_URL)
			   .methods(crow::HTTPMethod::Get)
				(
					[this](const Request& req, Response& resp) -> void
					{
						crow::mustache::set_base(crow::mustache::detail::get_global_template_base_directory_ref());

						WebRequestHandler(req, resp);
					}
				);
		}

	private:
		virtual void WebRequestHandler(const Request& req, Response& resp) = 0;
	};
}
// namespace AqualinkAutomate::Interfaces
