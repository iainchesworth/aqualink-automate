#pragma once

#include <memory>

#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENTVERSION_ROUTE_URL[] = "/api/equipment/version";

	class WebRoute_Equipment_Version : public Interfaces::IWebRoute<EQUIPMENTVERSION_ROUTE_URL>
	{
	public:
		WebRoute_Equipment_Version(Kernel::HubLocator& hub_locator);

	public:
        virtual HTTP::Message OnRequest(const HTTP::Request& req) final;

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
