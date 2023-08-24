#pragma once

#include <memory>

#include "http/webroute_types.h"
#include "interfaces/ishareableroute.h"
#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENTDEVICES_ROUTE_URL[] = "/api/equipment/devices";

	class WebRoute_Equipment_Devices : public Interfaces::IWebRoute<EQUIPMENTDEVICES_ROUTE_URL>, public Interfaces::IShareableRoute
	{
	public:
		WebRoute_Equipment_Devices(HTTP::Server& http_server, Kernel::HubLocator& hub_locator);

	public:
		void WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp);

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
