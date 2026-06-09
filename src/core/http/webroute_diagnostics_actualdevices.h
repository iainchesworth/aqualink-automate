#pragma once

#include <memory>

#include <nlohmann/json.hpp>

#include "interfaces/iwebroute.h"
#include "kernel/equipment_hub.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char DIAGNOSTICS_ACTUAL_DEVICES_ROUTE_URL[] = "/api/diagnostics/actual-devices";

	// Sibling of WebRoute_Diagnostics_Devices that reports only *real*
	// (non-emulated) bus-discovered devices. The JSON schema is identical to
	// the emulated-devices route; the only difference is the emulation filter
	// (see CollectDeviceDiagnostics in webroute_diagnostics_devices.h). Real
	// devices are passive snoopers and never transmit on the RS-485 bus, so
	// this endpoint carries zero bus risk.
	class WebRoute_Diagnostics_ActualDevices : public Interfaces::IWebRoute<DIAGNOSTICS_ACTUAL_DEVICES_ROUTE_URL>
	{
	public:
		WebRoute_Diagnostics_ActualDevices(Kernel::HubLocator& hub_locator);
		~WebRoute_Diagnostics_ActualDevices() override = default;

	public:
		HTTP::Message OnRequest(const HTTP::Request& req) final;

		// Collect diagnostic JSON for every *non-emulated* device in the hub.
		nlohmann::json CollectActualDiagnostics() const;

	private:
		std::shared_ptr<Kernel::EquipmentHub> m_EquipmentHub;
	};

}
// namespace AqualinkAutomate::HTTP
