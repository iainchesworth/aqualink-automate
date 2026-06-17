#pragma once

#include <memory>
#include <string>

#include <nlohmann/json.hpp>

#include "kernel/data_hub.h"
#include "kernel/statistics_hub.h"

namespace AqualinkAutomate::HTTP::JSON
{

	// Compute the user-facing display label for a device: the label_overrides entry for the
	// canonical label (else the canonical label itself), optionally suffixed with the
	// protocol-native id ("Pool Light (Aux5)") when show_aux_id_in_label is set and a
	// hardware_id is known. Shared by the equipment-devices and equipment-buttons routes so
	// both render identically.
	std::string ComputeDisplayLabel(const std::string& canonical_label, const std::string& hardware_id, const nlohmann::json& label_overrides, bool show_aux_id_in_label);

	nlohmann::json GenerateJson_Equipment_Devices(const std::shared_ptr<Kernel::DataHub>& data_hub, const nlohmann::json& label_overrides, bool show_aux_id_in_label = false);
	nlohmann::json GenerateJson_Equipment_Stats(const std::shared_ptr<Kernel::StatisticsHub>& statistics_hub);
	nlohmann::json GenerateJson_Equipment_Version(const std::shared_ptr<Kernel::DataHub>& data_hub);

}
// namespace AqualinkAutomate::HTTP::JSON
