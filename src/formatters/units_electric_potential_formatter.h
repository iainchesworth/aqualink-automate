#pragma once

#include <format>
#include <iostream>
#include <string>

#include "types/units_electric_potential.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Units::millivolt_quantity& obj);

}
// namespace std

template<>
struct std::formatter<AqualinkAutomate::Units::millivolt_quantity> : std::formatter<std::string>
{
	template<typename FormatContext>
	auto format(const AqualinkAutomate::Units::millivolt_quantity& value_in_mV, FormatContext& ctx) const
	{
		return std::vformat_to(ctx.out(), "{}", std::make_format_args(value_in_mV.value()));
	}
};
