#pragma once

#include <format>
#include <ostream>
#include <string>

#include "types/units_dimensionless.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	auto operator<<(std::ostream& os, const AqualinkAutomate::Units::ppm_quantity& obj) -> std::ostream&;

}
// namespace std

template<>
struct std::formatter<AqualinkAutomate::Units::ppm_quantity>
{
	std::formatter<double> m_Base;

	constexpr auto parse(std::format_parse_context& ctx)
	{
		return m_Base.parse(ctx);
	}

	template<typename FormatContext>
	auto format(const AqualinkAutomate::Units::ppm_quantity& value_in_ppm, FormatContext& ctx) const
	{
		m_Base.format(value_in_ppm.value(), ctx);
		return std::format_to(ctx.out(), " ppm");
	}
};
