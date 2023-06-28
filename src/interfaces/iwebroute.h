#pragma once

#include <functional>
#include <initializer_list>
#include <string>
#include <tuple>

#include <crow/app.h>
#include <crow/mustache.h>
#include <crow/routing.h>

#include "concepts/is_c_array.h"
#include "http/webroute_types.h"

namespace AqualinkAutomate::Interfaces
{

	template<const auto& ROUTE_URL, typename ROUTE_HANDLER = HTTP::RouteHandler>
	requires (Concepts::CArrayRef<decltype(ROUTE_URL)>)
	class IWebRoute
	{
	public:
		explicit IWebRoute(crow::SimpleApp& app, std::initializer_list<std::tuple<crow::HTTPMethod, ROUTE_HANDLER>> routes)
		{
			for (auto [method, handler] : routes)
			{
				// CROW_ROUTE(app, ROUTE_URL)
				app.template route<crow::black_magic::get_parameter_tag(ROUTE_URL)>(ROUTE_URL)
					.methods(method)
					(
						[this, handler](const HTTP::Request& req, HTTP::Response& resp, auto... args) -> void
						{
							crow::mustache::set_base(crow::mustache::detail::get_global_template_base_directory_ref());

							handler(req, resp, args...);
						}
					);
			}
		}
	};
}
// namespace AqualinkAutomate::Interfaces
