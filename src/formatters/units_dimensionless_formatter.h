#pragma once

#include <format>
#include <iostream>
#include <string>

#include "types/units_dimensionless.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Units::ppm_quantity& obj);

}
// namespace std

template<>
struct std::formatter<AqualinkAutomate::Units::ppm_quantity> : std::formatter<std::string>
{
	template<typename FormatContext>
	auto format(const AqualinkAutomate::Units::ppm_quantity& value_in_ppm, FormatContext& ctx) const
	{
		return std::vformat_to(ctx.out(), "{} ppm", std::make_format_args(value_in_ppm.value()));
	}
};
