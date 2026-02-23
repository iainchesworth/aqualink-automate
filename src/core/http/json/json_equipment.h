#pragma once

#include <memory>

#include <nlohmann/json.hpp>

#include "kernel/data_hub.h"
#include "kernel/statistics_hub.h"

namespace AqualinkAutomate::HTTP::JSON
{

	nlohmann::json GenerateJson_Equipment_Buttons(const std::shared_ptr<Kernel::DataHub>& data_hub);
	nlohmann::json GenerateJson_Equipment_Devices(const std::shared_ptr<Kernel::DataHub>& data_hub);
	nlohmann::json GenerateJson_Equipment_Stats(const std::shared_ptr<Kernel::StatisticsHub>& statistics_hub);
	nlohmann::json GenerateJson_Equipment_Version(const std::shared_ptr<Kernel::DataHub>& data_hub);

}
// namespace AqualinkAutomate::HTTP::JSON
