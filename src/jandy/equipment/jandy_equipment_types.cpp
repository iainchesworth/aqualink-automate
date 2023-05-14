#include <format>
#include <unordered_map>

#include "logging/logging.h"
#include "jandy/equipment/jandy_equipment_types.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Equipment
{
	static const std::unordered_map<JandyEquipmentTypes, std::string> JANDY_EQUIPMENT_TYPE_STRINGS =
	{
		{JandyEquipmentTypes::RS4_Only, "RS-4 Only"},
		{JandyEquipmentTypes::RS6_Only, "RS-6 Only"},
		{JandyEquipmentTypes::RS8_Only, "RS-8 Only"},
		{JandyEquipmentTypes::RS2_6_Dual, "RS-2/6 Dual"},
		{JandyEquipmentTypes::RS2_10_Dual, "RS-2/10 Dual"},
		{JandyEquipmentTypes::RS2_14_Dual, "RS-2/14 Dual"},
		{JandyEquipmentTypes::RS4_Combo, "RS-4 Combo"},
		{JandyEquipmentTypes::RS6_Combo, "RS-6 Combo"},
		{JandyEquipmentTypes::RS8_Combo, "RS-8 Combo"},
		{JandyEquipmentTypes::RS12_Combo, "RS-12 Combo"},
		{JandyEquipmentTypes::RS16_Combo, "RS-16 Combo"},
		{JandyEquipmentTypes::PD8_Only, "PD-8 Only"},
		{JandyEquipmentTypes::PD8_Combo, "PD-8 Combo"},
		{JandyEquipmentTypes::Unknown, "Unknown"},
	};


	JandyEquipmentTypes JandyEquipmentType_FromString(const std::string& equipment_type, JandyEquipmentTypes value_if_unknown)
	{
		for (auto& [elem_type, elem_string] : JANDY_EQUIPMENT_TYPE_STRINGS)
		{
			if (equipment_type == elem_string)
			{
				return elem_type;
			}
		}

		LogDebug(Channel::Equipment, std::format("Failed to convert a string to a JandyEquipmentTypes - no match found; string was: {}", equipment_type));
		return value_if_unknown;
	}

	std::string JandyEquipmentTypes_ToString(const JandyEquipmentTypes& equipment_type)
	{
		auto it = JANDY_EQUIPMENT_TYPE_STRINGS.find(equipment_type);
		if (JANDY_EQUIPMENT_TYPE_STRINGS.end() == it)
		{
			LogDebug(Channel::Equipment, std::format("Failed to convert an JandyEquipmentTypes to a string; value was: {}", static_cast<uint32_t>(equipment_type)));
			return JANDY_EQUIPMENT_TYPE_STRINGS.at(JandyEquipmentTypes::Unknown);
		}
		else
		{
			return it->second;
		}
	}

}
// namespace AqualinkAutomate::Equipment
