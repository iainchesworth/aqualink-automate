#pragma once

#include <memory>
#include <string_view>

#include "kernel/auxillary_devices/auxillary_device.h"

namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes
{

	std::string_view ConvertStatusToString(const AuxillaryDevice& device);
	std::string_view ConvertStatusToString(const std::shared_ptr<AuxillaryDevice>& device);

	// Returns true when the device carries a status that ConvertStatusToString can resolve.
	// Centralises the "does this device have a displayable status?" question now that the
	// per-category status traits use distinct keys (they previously shared a single key, so
	// callers could probe the generic StatusTrait as a catch-all marker).
	bool HasStatus(const AuxillaryDevice& device);
	bool HasStatus(const std::shared_ptr<AuxillaryDevice>& device);

}
// namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes
