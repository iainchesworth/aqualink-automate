#pragma once

#include <memory>

#include "http/webroute_types.h"
#include "interfaces/ishareableroute.h"
#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENTVERSION_ROUTE_URL[] = "/api/equipment/version";

	class WebRoute_Equipment_Version : public Interfaces::IWebRoute<EQUIPMENTVERSION_ROUTE_URL>, public Interfaces::IShareableRoute
	{
	public:
		WebRoute_Equipment_Version(HTTP::Server& http_server, Kernel::HubLocator& hub_locator);

	public:
		void WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp);

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
