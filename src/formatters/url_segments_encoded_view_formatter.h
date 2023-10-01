#pragma once

#include <format>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

#include <boost/url/segments_encoded_view.hpp>

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const boost::urls::segments_encoded_view& obj);

}
// namespace std

template<>
struct std::formatter<boost::urls::segments_encoded_view> : std::formatter<std::string>
{
	constexpr auto parse(format_parse_context& ctx) 
	{ 
		return ctx.begin(); 
	}

	template<typename FormatContext>
	auto format(const boost::urls::segments_encoded_view& sev, FormatContext& ctx) const
	{
		auto out = ctx.out();

		for (const auto& sev_elem : sev)
		{ 
			std::vformat_to(out, "/{}", std::make_format_args(std::string_view(sev_elem.data(), sev_elem.size())));
		}

		return out;
	}
};
