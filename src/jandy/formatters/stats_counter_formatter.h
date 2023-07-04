#pragma once

#include <format>
#include <iostream>
#include <string>

#include "utility/signalling_stats_counter.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Utility::StatsCounter& obj);

}
// namespace std

template<>
struct std::formatter<AqualinkAutomate::Utility::StatsCounter> : std::formatter<std::string>
{
	template<typename FormatContext>
	auto format(const AqualinkAutomate::Utility::StatsCounter& stats_counter, FormatContext& ctx) const
	{
		return std::vformat_to(ctx.out(), "{}", std::make_format_args(stats_counter.Count()));
	}
};
