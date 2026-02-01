#pragma once

#include <format>
#include <iostream>
#include <string>
#include <string_view>

#include <boost/beast/core/string.hpp>

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	auto operator<<(std::ostream& os, const boost::beast::string_view& obj) -> std::ostream&;

}
// namespace std

template<>
struct std::formatter<boost::beast::string_view> : std::formatter<std::string>
{
	template<typename FormatContext>
	auto format(const boost::beast::string_view& sv, FormatContext& ctx) const
	{
		const std::string_view v{ sv.data(), sv.size() };

		return std::vformat_to(ctx.out(), "{}", std::make_format_args(v));
	}
};
