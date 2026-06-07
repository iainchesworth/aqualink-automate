#pragma once

#include <memory>

#include <nlohmann/json.hpp>

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

		// Collect diagnostic JSON for every *emulated* device in the hub.
		// Exposed and side-effect free so the emulation filter can be unit
		// tested without driving a full HTTP request/response cycle.
		nlohmann::json CollectEmulatedDiagnostics() const;

	private:
		std::shared_ptr<Kernel::EquipmentHub> m_EquipmentHub;
	};

}
// namespace AqualinkAutomate::HTTP
