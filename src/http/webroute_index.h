#pragma once

#include <crow/app.h>

#include "interfaces/iwebroute.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char INDEX_ROUTE_URL[] = "/";

	class WebRoute_Index : public Interfaces::IWebRoute<INDEX_ROUTE_URL>
	{
	public:
		WebRoute_Index(crow::SimpleApp& app, const std::string& doc_root);

	public:
		void WebRequestHandler(const Request& req, Response& resp);
	};


}
// namespace AqualinkAutomate::HTTP
