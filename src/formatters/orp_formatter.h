#pragma once

#include <format>
#include <iostream>
#include <string>

#include "formatters/units_electric_potential_formatter.h"
#include "kernel/orp.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Kernel::ORP& obj);

}
// namespace std

template<>
struct std::formatter<AqualinkAutomate::Kernel::ORP> : std::formatter<std::string>
{
	template<typename FormatContext>
	auto format(const AqualinkAutomate::Kernel::ORP& orp, FormatContext& ctx) const
	{
		return std::vformat_to(ctx.out(), "{}", std::make_format_args(orp()));
	}
};
