#pragma once

#include <memory>
#include <string_view>

#include "kernel/auxillary_devices/auxillary_device.h"

namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes
{

	std::string_view ConvertStatusToString(const AuxillaryDevice& device);
	std::string_view ConvertStatusToString(std::shared_ptr<AuxillaryDevice> device);

}
// namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes
