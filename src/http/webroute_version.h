#pragma once

#include <string>

#include <crow/app.h>

#include "interfaces/iwebroute.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char VERSION_ROUTE_URL[] = "/api/version";

	class WebRoute_Version : public Interfaces::IWebRoute<VERSION_ROUTE_URL>
	{
	public:
		WebRoute_Version(crow::SimpleApp& app, const std::string& doc_root);

	public:
		void WebRequestHandler(const Request& req, Response& resp);
	};

}
// namespace AqualinkAutomate::HTTP