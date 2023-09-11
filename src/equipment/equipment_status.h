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
		inline static constexpr std::string_view SOURCE_NAME{ "my_source" };
		inline static constexpr std::string_view SOURCE_TYPE{ "equipment" };

	public:
		virtual std::string_view SourceName() const final
		{
			return SOURCE_NAME;
		}

		virtual std::string_view SourceType() const final
		{
			return SOURCE_TYPE;
		}

		virtual std::string_view StatusType() const final
		{
			return m_StatusType;
		}

	private:
		const std::string_view m_StatusType{ EQUPIMENT_STATUS_STRING };
	};

	inline constexpr char EQUIPMENTSTATUS_UNKNOWN_NAME[] = "Unknown";
	class EquipmentStatus_Unknown : public EquipmentStatus<EQUIPMENTSTATUS_UNKNOWN_NAME>
	{
	};

}
// namespace AqualinkAutomate::Equipment
