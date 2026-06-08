#pragma once

#include "interfaces/status_tag.h"

namespace AqualinkAutomate::Equipment
{

	inline constexpr char EQUIPMENTSTATUS_SOURCE_TYPE[] = "equipment";

	/// Equipment-side status tag: a StatusTag whose source-type text is "equipment".
	template<const auto& EQUIPMENT_STATUS_STRING>
	using EquipmentStatus = Interfaces::StatusTag<EQUIPMENT_STATUS_STRING, EQUIPMENTSTATUS_SOURCE_TYPE>;

	inline constexpr char EQUIPMENTSTATUS_UNKNOWN_NAME[] = "Unknown";
	class EquipmentStatus_Unknown : public EquipmentStatus<EQUIPMENTSTATUS_UNKNOWN_NAME>
	{
	};

}
// namespace AqualinkAutomate::Equipment
