#pragma once

#include <functional>
#include <string>

#include <crow/app.h>
#include <crow/routing.h>

namespace AqualinkAutomate::Interfaces
{

	template<const char* ROUTE_URL>
	class IWebRoute
	{
	public:
		using Request = crow::request;
		using Response = crow::response;
		using RouteParams = crow::routing_params;

	public:
		explicit IWebRoute(crow::SimpleApp& app, const std::string& doc_root) : 
			m_DocRoot(doc_root)
		{
			app.route_dynamic(ROUTE_URL)
			   .methods(crow::HTTPMethod::Get)
				(
					[this](const Request& req, Response& resp) -> void
					{
						WebRequestHandler(req, resp);
					}
				);
		}

	protected:
		const std::string& m_DocRoot;

	private:
		virtual void WebRequestHandler(const Request& req, Response& resp) = 0;
	};
}
// namespace AqualinkAutomate::Interfaces
