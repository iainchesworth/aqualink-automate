#pragma once

#include <memory>
#include <string_view>

#include "kernel/auxillary_base.h"

namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes
{

	std::string_view ConvertStatusToString(std::shared_ptr<AuxillaryBase> device);

}
// namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes
