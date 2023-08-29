#pragma once

#include <memory>
#include <string_view>

#include "concepts/is_c_array.h"
#include "interfaces/istatus.h"

namespace AqualinkAutomate::Equipment
{

	template<const auto& EQUPIMENT_STATUS_STRING>
	requires (Concepts::CArray<decltype(EQUPIMENT_STATUS_STRING)>)
	class EquipmentStatus : public Interfaces::IStatus
	{
	public:
		virtual std::string_view Name() const final
		{
			return m_Name;
		}

	private:
		const std::string_view m_Name{ EQUPIMENT_STATUS_STRING };
	};

	inline constexpr char EQUIPMENTSTATUS_UNKNOWN_NAME[] = "Unknown";
	class EquipmentStatus_Unknown : public EquipmentStatus<EQUIPMENTSTATUS_UNKNOWN_NAME>
	{
	};

}
// namespace AqualinkAutomate::Equipment
