#pragma once

#include <format>
#include <iostream>
#include <string>

#include <boost/format.hpp>

#include "kernel/ph.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Kernel::pH& obj);

}
// namespace std

template<>
struct std::formatter<AqualinkAutomate::Kernel::pH> : std::formatter<std::string>
{
	template<typename FormatContext>
	auto format(const AqualinkAutomate::Kernel::pH& pH, FormatContext& ctx) const
	{
		return std::vformat_to(ctx.out(), "{:.1f}", std::make_format_args(pH()));
	}
};
