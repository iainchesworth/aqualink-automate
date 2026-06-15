#pragma once

#include <memory>

#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"
#include "kernel/preferences_hub.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENTDEVICES_ROUTE_URL[] = "/api/equipment/devices";

	class WebRoute_Equipment_Devices : public Interfaces::IWebRoute<EQUIPMENTDEVICES_ROUTE_URL>
	{
	public:
		WebRoute_Equipment_Devices(Kernel::HubLocator& hub_locator);

	public:
        HTTP::Response OnRequest(const HTTP::Request& req) final;

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
		std::shared_ptr<Kernel::PreferencesHub> m_PreferencesHub{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
