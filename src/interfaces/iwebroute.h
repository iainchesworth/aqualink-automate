#pragma once

#include <functional>
#include <initializer_list>
#include <string>
#include <tuple>

#include "concepts/is_c_array.h"
#include "http/webroute_types.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Interfaces
{

	template<const auto& ROUTE_URL, typename ROUTE_HANDLER = HTTP::RouteHandler>
	requires (Concepts::CArray<decltype(ROUTE_URL)>)
	class IWebRoute
	{
	public:
		explicit IWebRoute(HTTP::Server& http_server, std::initializer_list<std::tuple<HTTP::Methods, ROUTE_HANDLER>> routes) :
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

				auto route_handler = [this, route_tuple](HTTP::Request& req, HTTP::Response& resp) -> void
				{
					std::get<ROUTE_HANDLER>(route_tuple)(req, resp);
				};

				switch (std::get<HTTP::Methods>(route_tuple))
				{
				case HTTP::Methods::GET: 
					http_server.set_http_handler<HTTP::Methods::GET>(ROUTE_URL, route_handler); 
					break;

				case HTTP::Methods::POST: 
					http_server.set_http_handler<HTTP::Methods::POST>(ROUTE_URL, route_handler); 
					break;
				
				case HTTP::Methods::UNKNOW:
				case HTTP::Methods::DEL:
				case HTTP::Methods::HEAD:
				case HTTP::Methods::PUT:
				case HTTP::Methods::PATCH:
				case HTTP::Methods::CONNECT:
				case HTTP::Methods::OPTIONS:
				case HTTP::Methods::TRACE:
				default:
					LogWarning(Channel::Web, "Attempted to configure an unsupported web method");
				}
			}
		}

	private:
		std::vector<std::tuple<HTTP::Methods, ROUTE_HANDLER>> m_Routes;
	};

}
// namespace AqualinkAutomate::Interfaces
