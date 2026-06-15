#pragma once

#include <memory>

#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"
#include "kernel/preferences_hub.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENTBUTTONS_ROUTE_URL[] = "/api/equipment/buttons";

	class WebRoute_Equipment_Buttons : public Interfaces::IWebRoute<EQUIPMENTBUTTONS_ROUTE_URL>
	{
	public:
        WebRoute_Equipment_Buttons(Kernel::HubLocator& hub_locator);

    public:
        HTTP::Response OnRequest(const HTTP::Request& req) final;

	public:
        HTTP::Response ButtonCollection_GetHandler(const HTTP::Request& req);
        HTTP::Response ButtonCollection_PostHandler(const HTTP::Request& req);

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
		std::shared_ptr<Kernel::PreferencesHub> m_PreferencesHub{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
