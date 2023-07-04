#pragma once

#include <crow/app.h>

#include "http/webroute_types.h"
#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char PAGE_INDEX_ROUTE_URL[] = "/";

	class WebRoute_Page_Index : public Interfaces::IWebRoute<PAGE_INDEX_ROUTE_URL>
	{
	public:
		WebRoute_Page_Index(crow::SimpleApp& app, const Kernel::DataHub& data_hub);

	public:
		void WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp);

	private:
		const Kernel::DataHub& m_DataHub;
	};

}
// namespace AqualinkAutomate::HTTP
