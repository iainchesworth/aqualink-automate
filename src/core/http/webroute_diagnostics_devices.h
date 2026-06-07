#pragma once

#include <memory>

#include "interfaces/iwebroute.h"
#include "kernel/equipment_hub.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char DIAGNOSTICS_DEVICES_ROUTE_URL[] = "/api/diagnostics/emulated-devices";

	class WebRoute_Diagnostics_Devices : public Interfaces::IWebRoute<DIAGNOSTICS_DEVICES_ROUTE_URL>
	{
	public:
		WebRoute_Diagnostics_Devices(Kernel::HubLocator& hub_locator);
		~WebRoute_Diagnostics_Devices() override = default;

	public:
		HTTP::Message OnRequest(const HTTP::Request& req) final;

	private:
		std::shared_ptr<Kernel::EquipmentHub> m_EquipmentHub;
	};

}
// namespace AqualinkAutomate::HTTP
