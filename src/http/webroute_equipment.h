#pragma once

#include <memory>

#include "http/webroute_types.h"
#include "interfaces/ishareableroute.h"
#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"
#include "kernel/statistics_hub.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENT_ROUTE_URL[] = "/api/equipment";

	class WebRoute_Equipment : public Interfaces::IWebRoute<EQUIPMENT_ROUTE_URL>, public Interfaces::IShareableRoute
	{
	public:
		WebRoute_Equipment(HTTP::Server& http_server, Kernel::HubLocator& hub_locator);

	public:
		void WebGetRequestHandler(const HTTP::Request& req, HTTP::Response& resp);

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
		std::shared_ptr<Kernel::StatisticsHub> m_StatisticsHub{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
