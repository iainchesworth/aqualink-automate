#pragma once

#include <memory>

#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"
#include "kernel/statistics_hub.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENT_ROUTE_URL[] = "/api/equipment";

	class WebRoute_Equipment : public Interfaces::IWebRoute<EQUIPMENT_ROUTE_URL>
	{
	public:
		WebRoute_Equipment(Kernel::HubLocator& hub_locator);

	public:
        virtual HTTP::Message OnRequest(const HTTP::Request& req) final;

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
		std::shared_ptr<Kernel::StatisticsHub> m_StatisticsHub{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
