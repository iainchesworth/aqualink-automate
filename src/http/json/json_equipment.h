#pragma once

#include <nlohmann/json.hpp>

#include "kernel/data_hub.h"
#include "kernel/statistics_hub.h"

namespace AqualinkAutomate::HTTP::JSON
{

	nlohmann::json GenerateJson_Equipment_Buttons(const Kernel::DataHub& data_hub);
	nlohmann::json GenerateJson_Equipment_Devices(const Kernel::DataHub& data_hub);
	nlohmann::json GenerateJson_Equipment_Stats(const Kernel::StatisticsHub& statistics_hub);
	nlohmann::json GenerateJson_Equipment_Version(const Kernel::DataHub& data_hubt);

}
// namespace AqualinkAutomate::HTTP::JSON
