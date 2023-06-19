#pragma once

#include <nlohmann/json.hpp>

#include "jandy/equipment/jandy_equipment.h"

using namespace AqualinkAutomate::Equipment;

namespace AqualinkAutomate::HTTP::JSON
{

	nlohmann::json GenerateJson_JandyEquipment_Buttons(const JandyEquipment& jandy_equipment);
	nlohmann::json GenerateJson_JandyEquipment_Devices(const JandyEquipment& jandy_equipment);
	nlohmann::json GenerateJson_JandyEquipment_Stats(const JandyEquipment& jandy_equipment);
	nlohmann::json GenerateJson_JandyEquipment_Version(const JandyEquipment& jandy_equipment);

}
// namespace AqualinkAutomate::HTTP::JSON
