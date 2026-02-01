#pragma once

#include <memory>

#include <nlohmann/json.hpp>

#include "kernel/data_hub.h"
#include "kernel/statistics_hub.h"

namespace AqualinkAutomate::HTTP::JSON
{

	nlohmann::json GenerateJson_Equipment_Buttons(std::shared_ptr<Kernel::DataHub> data_hub);
	nlohmann::json GenerateJson_Equipment_Devices(std::shared_ptr<Kernel::DataHub> data_hub);
	nlohmann::json GenerateJson_Equipment_Stats(std::shared_ptr<Kernel::StatisticsHub> statistics_hub);
	nlohmann::json GenerateJson_Equipment_Version(std::shared_ptr<Kernel::DataHub> data_hub);

}
// namespace AqualinkAutomate::HTTP::JSON
