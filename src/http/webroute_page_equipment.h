#pragma once

#include <memory>

#include "interfaces/iwebpageroute.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char PAGE_EQUIPMENT_ROUTE_URL[] = "/equipment";
	inline constexpr char PAGE_EQUIPMENT_TEMPLATE[] = "templates/equipment.html.mustache";

	class WebRoute_Page_Equipment : public Interfaces::IWebPageRoute<PAGE_EQUIPMENT_ROUTE_URL, PAGE_EQUIPMENT_TEMPLATE>
	{
	public:
		WebRoute_Page_Equipment(Kernel::HubLocator& hub_locator);

	public:
        virtual HTTP::Message OnRequest(HTTP::Request req) final;

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
