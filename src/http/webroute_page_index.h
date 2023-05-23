#pragma once

#include <string>

#include <crow/app.h>

#include "interfaces/iwebroute.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char PAGE_INDEX_ROUTE_URL[] = "/";

	class WebRoute_Page_Index : public Interfaces::IWebRoute<PAGE_INDEX_ROUTE_URL>
	{
	public:
		WebRoute_Page_Index(crow::SimpleApp& app, const std::string& doc_root);

	public:
		void WebRequestHandler(const Request& req, Response& resp);
	};

}
// namespace AqualinkAutomate::HTTP
