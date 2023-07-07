#pragma once

#include "http/webroute_types.h"
#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char PAGE_EQUIPMENT_ROUTE_URL[] = "/equipment";

	class WebRoute_Page_Equipment : public Interfaces::IWebRoute<PAGE_EQUIPMENT_ROUTE_URL>
	{
	public:
		WebRoute_Page_Equipment(HTTP::Server& http_server, const Kernel::DataHub& data_hub);

	public:
		void WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp);

	private:
		const Kernel::DataHub& m_DataHub;
	};

}
// namespace AqualinkAutomate::HTTP
