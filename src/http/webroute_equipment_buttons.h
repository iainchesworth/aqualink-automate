#pragma once

#include <functional>
#include <memory>
#include <string_view>
#include <vector>

#include <mstch/mstch.hpp>
#include <nlohmann/json.hpp>

#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENTBUTTONS_ROUTE_URL[] = "/api/equipment/buttons";

	class WebRoute_Equipment_Buttons : public Interfaces::IWebRoute<EQUIPMENTBUTTONS_ROUTE_URL>
	{
	public:
        WebRoute_Equipment_Buttons(Kernel::HubLocator& hub_locator);

    public:
        virtual HTTP::Message OnRequest(HTTP::Request req) final;

	public:
        HTTP::Message ButtonCollection_GetHandler(HTTP::Request req);
        HTTP::Message ButtonCollection_PostHandler(HTTP::Request req);

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
