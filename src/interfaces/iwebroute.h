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
		explicit IWebRoute(crow::SimpleApp& app, std::initializer_list<std::tuple<crow::HTTPMethod, ROUTE_HANDLER>> routes) :
			m_Routes(routes)
		{
			for (auto& route_tuple : m_Routes)
			{
				// Using a structured binding for the above e.g. auto [method, handler] : m_Routes 
				// will result in an error when compiling with clang (but not MSVC nor gcc) due to
				// the use of handler (which is the structured binding subobject) being ultimately
				// passed to a lambda function which is called with the route is triggered.
				//
				// Note that clang is *actually* correct here although it "works" in MSVC and gcc.

				// CROW_ROUTE(app, ROUTE_URL)
				app.template route<crow::black_magic::get_parameter_tag(ROUTE_URL)>(ROUTE_URL)
					.methods(std::get<crow::HTTPMethod>(route_tuple))
					(
						[this, route_tuple](const HTTP::Request& req, HTTP::Response& resp, auto... args) -> void
						{
							crow::mustache::set_base(crow::mustache::detail::get_global_template_base_directory_ref());

							std::get<ROUTE_HANDLER>(route_tuple)(req, resp, args...);
						}
					);
			}
		}

	public:
		void Handler(const HTTP::Request& req, HTTP::Response& resp, auto... args)
		{
		}

	private:
		std::vector<std::tuple<crow::HTTPMethod, ROUTE_HANDLER>> m_Routes;
	};
}
// namespace AqualinkAutomate::Interfaces
