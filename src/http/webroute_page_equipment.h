#pragma once

#include <memory>

#include "http/webroute_types.h"
#include "interfaces/ishareableroute.h"
#include "interfaces/iwebpageroute.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char PAGE_EQUIPMENT_ROUTE_URL[] = "/equipment";
	inline constexpr char PAGE_EQUIPMENT_TEMPLATE[] = "templates/equipment.html.mustache";

	class WebRoute_Page_Equipment : public Interfaces::IWebPageRoute<PAGE_EQUIPMENT_ROUTE_URL, PAGE_EQUIPMENT_TEMPLATE>, public Interfaces::IShareableRoute
	{
	public:
		WebRoute_Page_Equipment(HTTP::Server& http_server, Kernel::HubLocator& hub_locator);

	public:
		virtual void WebRequestHandler(HTTP::Request& req, HTTP::Response& resp) override;

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
