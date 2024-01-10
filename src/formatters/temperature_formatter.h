#pragma once

#include <format>
#include <iostream>
#include <string>
#include <string_view>

#include <boost/format.hpp>

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

	template<class ParseContext>
	constexpr auto parse(ParseContext& ctx) -> ParseContext::iterator
	{
		auto it = ctx.begin();
		if ((ctx.end() != it) && ('}' != *it))
		{
			// Parse and skip over any filler characters

			if (':' == *it)
			{
				++it;
			}

			///FIXME - Add precision support to the std::formatter<Kernel::Temperature>

			// Parse and extract the required temperature units

			switch (*it)
			{
			case 'C':
				display_units = AqualinkAutomate::Kernel::TemperatureUnits::Celsius;
				++it;
				break;

			case 'F':
				display_units = AqualinkAutomate::Kernel::TemperatureUnits::Fahrenheit;
				++it;
				break;

			default:
				throw std::format_error("std::formatter<AqualinkAutomate::Kernel::Temperature> -> Unsupported temperature unit");
			}

			// Validate that there are no further characters to be parsed.

			if ((ctx.end() != it) && ('}' != *it))
			{
				throw std::format_error("std::formatter<AqualinkAutomate::Kernel::Temperature> -> Invalid format specifier");
			}
		}

		return it;
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
		std::vformat_to(ctx.out(), "{:.{}f}", std::make_format_args(temp_value, 0));
		std::vformat_to(ctx.out(), "\u00B0{}", std::make_format_args(temp_units));

		return ctx.out();
	}
};
