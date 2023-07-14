#pragma once

#include <format>
#include <iostream>
#include <string>

#include <tl/expected.hpp>

#include "jandy/utility/string_conversion/temperature.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Utility::Temperature& obj);

}
// namespace std

template<>
struct std::formatter<AqualinkAutomate::Utility::Temperature::Units> : std::formatter<std::string>
{
	template<typename FormatContext>
	auto format(const AqualinkAutomate::Utility::Temperature::Units& units, FormatContext& ctx) const
	{
		static const std::string_view CELSIUS{ "C" };
		static const std::string_view FARENHEIT{ "F" };
		static const std::string_view UNKNOWN{ "?" };

		auto ctx_it = ctx.out();

		switch (units)
		{
		case AqualinkAutomate::Utility::Temperature::Units::Celsius:
			std::copy(CELSIUS.begin(), CELSIUS.end(), ctx_it);
			break;

		case AqualinkAutomate::Utility::Temperature::Units::Farenheit:
			std::copy(FARENHEIT.begin(), FARENHEIT.end(), ctx_it);
			break;

		case AqualinkAutomate::Utility::Temperature::Units::Unknown:
			[[fallthrough]];
		default:
			std::copy(UNKNOWN.begin(), UNKNOWN.end(), ctx_it);
			break;
		};
		
		return ctx_it;
	}
};

template<>
struct std::formatter<AqualinkAutomate::Utility::Temperature> : std::formatter<std::string>
{
	template<typename FormatContext>
	auto format(const AqualinkAutomate::Utility::Temperature& temperature, FormatContext& ctx) const
	{
		try
		{
			auto temps = temperature().value();
			auto units = temperature.TemperatureUnits().value();

			return std::vformat_to(ctx.out(), "{}\u00B0{}", std::make_format_args(temps, units));
		}
		catch (const tl::bad_expected_access<boost::system::error_code>& ex_bea)
		{
			static const std::string_view UNKNOWN_TEMP{ "TEMP=??" };

			auto ctx_it = ctx.out();

			std::copy(UNKNOWN_TEMP.begin(), UNKNOWN_TEMP.end(), ctx_it);

			return ctx_it;
		}
	}
};
