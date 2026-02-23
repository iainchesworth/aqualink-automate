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
	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Units::volt_quantity& obj);

}
// namespace std

template<>
struct std::formatter<AqualinkAutomate::Units::millivolt_quantity>
{
	std::formatter<double> m_Base;

	constexpr auto parse(std::format_parse_context& ctx)
	{
		return m_Base.parse(ctx);
	}

	template<typename FormatContext>
	auto format(const AqualinkAutomate::Units::millivolt_quantity& value_in_mV, FormatContext& ctx) const
	{
		m_Base.format(value_in_mV.value(), ctx);
		return std::format_to(ctx.out(), "mV");
	}
};

template<>
struct std::formatter<AqualinkAutomate::Units::volt_quantity>
{
	std::formatter<double> m_Base;

	constexpr auto parse(std::format_parse_context& ctx)
	{
		return m_Base.parse(ctx);
	}

	template<typename FormatContext>
	auto format(const AqualinkAutomate::Units::volt_quantity& value_in_V, FormatContext& ctx) const
	{
		m_Base.format(value_in_V.value(), ctx);
		return std::format_to(ctx.out(), "V");
	}
};
