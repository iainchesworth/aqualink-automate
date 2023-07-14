#pragma once

#include <format>
#include <iostream>
#include <string>

#include <tl/expected.hpp>

#include "formatters/orp_formatter.h"
#include "formatters/ph_formatter.h"
#include "jandy/utility/string_conversion/chemistry.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Utility::Chemistry& obj);

}
// namespace std

template<>
struct std::formatter<AqualinkAutomate::Utility::Chemistry> : std::formatter<std::string>
{
	template<typename FormatContext>
	auto format(const AqualinkAutomate::Utility::Chemistry& chemistry, FormatContext& ctx) const
	{
		try
		{
			auto orp = chemistry.ORP().value();
			auto ph = chemistry.PH().value();

			return std::vformat_to(ctx.out(), "ORP={} PH={}", std::make_format_args(orp, ph));
		}
		catch (const tl::bad_expected_access<boost::system::error_code>& ex_bea)
		{
			static const std::string_view UNKNOWN_CHEMISTRY{ "CHEM=??" };

			auto ctx_it = ctx.out();

			std::copy(UNKNOWN_CHEMISTRY.begin(), UNKNOWN_CHEMISTRY.end(), ctx_it);

			return ctx_it;
		}
	}
};
