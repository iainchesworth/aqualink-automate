#pragma once

#include <format>
#include <iostream>
#include <string>
#include <string_view>

#include "kernel/temperature.h"
#include "utility/to_string.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Kernel::Temperature& obj);

}
// namespace std

template<>
struct std::formatter<AqualinkAutomate::Kernel::Temperature>
{
	// Set the formatting defaults
	AqualinkAutomate::Kernel::TemperatureUnits display_units{ AqualinkAutomate::Kernel::TemperatureUnits::Celsius };
	uint32_t display_precision = 0;

	constexpr auto parse(std::format_parse_context& ctx) -> std::format_parse_context::iterator
	{
		auto it = ctx.begin();

		if (it == ctx.end() || *it == '}')
		{
			return it;
		}

		// Parse optional precision: .N
		if (*it == '.')
		{
			++it;
			uint32_t precision = 0;
			while (it != ctx.end() && *it >= '0' && *it <= '9')
			{
				precision = precision * 10 + static_cast<uint32_t>(*it - '0');
				++it;
			}
			display_precision = precision;
		}

		if (it == ctx.end() || *it == '}')
		{
			return it;
		}

		if (*it == 'C')
		{
			display_units = AqualinkAutomate::Kernel::TemperatureUnits::Celsius;
			return ++it;
		}

		if (*it == 'F')
		{
			display_units = AqualinkAutomate::Kernel::TemperatureUnits::Fahrenheit;
			return ++it;
		}

		throw std::format_error("Unsupported temperature unit format specifier");
	}

	template<class FmtContext>
	auto format(const AqualinkAutomate::Kernel::Temperature& temperature, FmtContext& ctx) const -> FmtContext::iterator
	{
		double temp_value = 0.0f;
		std::string temp_units = "?";

		switch (display_units)
		{
		case AqualinkAutomate::Kernel::TemperatureUnits::Celsius:
			temp_value = temperature.InCelsius().value();
			temp_units = "C";
			break;

		case AqualinkAutomate::Kernel::TemperatureUnits::Fahrenheit:
			temp_value = temperature.InFahrenheit().value();
			temp_units = "F";
			break;
		}

		// Output the degrees symbol and units symbol.
		return std::format_to(ctx.out(), "{:.{}f}\u00B0{}", temp_value, display_precision, temp_units);
	}
};
