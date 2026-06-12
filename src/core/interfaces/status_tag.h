#pragma once

#include <string_view>

#include "concepts/is_c_array.h"
#include "interfaces/istatus.h"

namespace AqualinkAutomate::Interfaces
{

	/// Shared compile-time status "tag" base for the family of status value-types
	/// (device statuses, equipment statuses, ...).  A concrete status is just a
	/// distinct C++ type carrying two pieces of compile-time text:
	///
	///   - SOURCE_TYPE_STRING : the category the status belongs to ("device",
	///                          "equipment", ...).
	///   - STATUS_STRING      : the status label itself ("Normal", "Unknown", ...).
	///
	/// Both are passed as C-array NTTPs (e.g. `inline constexpr char NAME[] = "...";`).
	///
	/// SourceName() is intentionally NOT final so a concrete device/equipment can
	/// override it with its own instance name; the default simply echoes the source
	/// type rather than a hard-coded placeholder.
	template<const auto& STATUS_STRING, const auto& SOURCE_TYPE_STRING>
		requires (Concepts::CArray<decltype(STATUS_STRING)> && Concepts::CArray<decltype(SOURCE_TYPE_STRING)>)
	class StatusTag : public IStatus
	{
	public:
		std::string_view SourceName() const override
		{
			return m_SourceType;
		}

		std::string_view SourceType() const final
		{
			return m_SourceType;
		}

		std::string_view StatusType() const final
		{
			return m_StatusType;
		}

	private:
		const std::string_view m_StatusType{ STATUS_STRING };
		const std::string_view m_SourceType{ SOURCE_TYPE_STRING };
	};

}
// namespace AqualinkAutomate::Interfaces
