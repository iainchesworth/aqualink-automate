#pragma once

#include <memory>

#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENTDEVICES_ROUTE_URL[] = "/api/equipment/devices";

	class WebRoute_Equipment_Devices : public Interfaces::IWebRoute<EQUIPMENTDEVICES_ROUTE_URL>
	{
	public:
		WebRoute_Equipment_Devices(Kernel::HubLocator& hub_locator);

	public:
        virtual HTTP::Message OnRequest(const HTTP::Request& req) final;

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
