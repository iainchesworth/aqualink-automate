#pragma once

#include <format>
#include <iostream>
#include <string>
#include <string_view>

#include <boost/format.hpp>

#include "kernel/temperature.h"

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
struct std::formatter<AqualinkAutomate::Kernel::Temperature> : std::formatter<std::string>
{
	// Set the formatting defaults

	AqualinkAutomate::Kernel::TemperatureUnits display_units{ AqualinkAutomate::Kernel::TemperatureUnits::Celsius };
	uint8_t display_precision{ 0 };

	constexpr auto parse(format_parse_context& ctx)
	{
		auto it = ctx.begin();

		if (it != ctx.end()) 
		{
			// Get the preferred temperature units....
			switch (*it)
			{
			case 'C':
				display_units = AqualinkAutomate::Kernel::TemperatureUnits::Celsius;
				it++;
				break;

			case 'F':
				display_units = AqualinkAutomate::Kernel::TemperatureUnits::Fahrenheit;
				it++;
				break;
			}

			// Get the preferred precision....
			if (it != ctx.end() && *it == ':') 
			{
				++it;
			
				if (it != ctx.end() && isdigit(static_cast<unsigned char>(*it)))
				{
					display_precision = *it++ - '0';
				}
			}
		}

		return it;
	}

	template<typename FormatContext>
	auto format(const AqualinkAutomate::Kernel::Temperature& temperature, FormatContext& ctx) const
	{
		double temp_value = 0.0f;
		std::string_view temp_units = "?";

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

		return std::vformat_to(ctx.out(), "{:.{}f}\u00B0{}", std::make_format_args(temp_value, display_precision, temp_units));
	}
};
