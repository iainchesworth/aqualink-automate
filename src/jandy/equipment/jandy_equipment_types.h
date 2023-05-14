#pragma once

#include <string>

namespace AqualinkAutomate::Equipment
{

	enum class JandyEquipmentTypes
	{
		RS4_Only,
		RS6_Only,
		RS8_Only,

		RS2_6_Dual,
		RS2_10_Dual,
		RS2_14_Dual,

		RS4_Combo,
		RS6_Combo,
		RS8_Combo,
		RS12_Combo,
		RS16_Combo,

		PD8_Only,
		PD8_Combo,

		Unknown
	};

	JandyEquipmentTypes JandyEquipmentType_FromString(const std::string& equipment_type, JandyEquipmentTypes value_if_unknown = JandyEquipmentTypes::Unknown);
	std::string JandyEquipmentTypes_ToString(const JandyEquipmentTypes& equipment_type);

}
// namespace AqualinkAutomate::Equipment
