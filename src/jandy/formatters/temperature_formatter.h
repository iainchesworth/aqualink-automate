#pragma once

#include <format>
#include <iostream>
#include <string>

#include <tl/expected.hpp>

#include "formatters/temperature_formatter.h"
#include "jandy/utility/string_conversion/temperature_string_converter.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Utility::TemperatureStringConverter& obj);

}
// namespace std

template<>
struct std::formatter<AqualinkAutomate::Utility::TemperatureStringConverter> : std::formatter<AqualinkAutomate::Kernel::Temperature>
{
	template<class ParseContext>
	constexpr ParseContext::iterator parse(ParseContext& ctx)
	{
		return std::formatter<AqualinkAutomate::Kernel::Temperature>::parse(ctx);
	}

	template<typename FormatContext>
	auto format(const AqualinkAutomate::Utility::TemperatureStringConverter& temperature, FormatContext& ctx) const
	{
		std::ranges::copy(std::string_view{ "TEMP=" }, ctx.out());

		try
		{
			return std::formatter<AqualinkAutomate::Kernel::Temperature>::format(temperature().value(), ctx);
		}
		catch (const tl::bad_expected_access<boost::system::error_code>& ex_bea)
		{
			return std::ranges::copy(std::string_view{ "??" }, ctx.out()).out;
		}
	}
};
