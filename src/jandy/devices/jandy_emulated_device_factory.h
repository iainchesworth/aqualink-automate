#pragma once

#include <memory>

#include "devices/jandy_device_id.h"
#include "devices/jandy_emulated_device_types.h"
#include "interfaces/idevice.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::Devices
{

	// Construct an EMULATED Jandy controller/peripheral of `type` bound to bus id `id`.
	// Returns nullptr for Unknown/unsupported types. This is the single place that maps an
	// emulated-device type to a concrete device, so the static (CLI-configured) path in
	// Jandy::Configure and the auto-startup coordinator build devices identically.
	std::unique_ptr<Interfaces::IDevice> MakeEmulatedDevice(JandyEmulatedDeviceTypes type, JandyDeviceId id, Kernel::HubLocator& hub_locator);

}
// namespace AqualinkAutomate::Devices
