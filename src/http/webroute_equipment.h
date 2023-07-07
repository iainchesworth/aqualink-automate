#pragma once

#include "http/webroute_types.h"
#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"
#include "kernel/statistics_hub.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char EQUIPMENT_ROUTE_URL[] = "/api/equipment";

	class WebRoute_Equipment : public Interfaces::IWebRoute<EQUIPMENT_ROUTE_URL>
	{
	public:
		WebRoute_Equipment(HTTP::Server& http_server, const Kernel::DataHub& data_hub, const Kernel::StatisticsHub& statistics_hub);

	public:
		void WebGetRequestHandler(const HTTP::Request& req, HTTP::Response& resp);

	private:
		const Kernel::DataHub& m_DataHub;
		const Kernel::StatisticsHub& m_StatisticsHub;
	};

}
// namespace AqualinkAutomate::HTTP
