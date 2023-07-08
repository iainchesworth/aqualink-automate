#pragma once

#include "http/webroute_types.h"
#include "interfaces/iwebpageroute.h"
#include "kernel/data_hub.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char PAGE_EQUIPMENT_ROUTE_URL[] = "/equipment";
	constexpr const char PAGE_EQUIPMENT_TEMPLATE[] = "templates/equipment.html.mustache";

	class WebRoute_Page_Equipment : public Interfaces::IWebPageRoute<PAGE_EQUIPMENT_ROUTE_URL, PAGE_EQUIPMENT_TEMPLATE>
	{
	public:
		WebRoute_Page_Equipment(HTTP::Server& http_server, const Kernel::DataHub& data_hub);

	public:
		virtual void WebRequestHandler(HTTP::Request& req, HTTP::Response& resp) override;

	private:
		const Kernel::DataHub& m_DataHub;
	};

}
// namespace AqualinkAutomate::HTTP
