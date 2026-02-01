#pragma once

#include <format>
#include <iostream>
#include <string>

#include "kernel/ph.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Kernel::pH& obj);

}
// namespace std

template<>
struct std::formatter<AqualinkAutomate::Kernel::pH>
{
	mutable std::formatter<boost::float32_t> m_Base;

	constexpr auto parse(std::format_parse_context& ctx)
	{
		return ctx.begin();
	}

	template<typename FormatContext>
	auto format(const AqualinkAutomate::Kernel::pH& pH, FormatContext& ctx) const
	{
		std::basic_format_parse_context<char> parse_ctx(std::string_view(".01f"));

		m_Base.parse(parse_ctx);
		
		return m_Base.format(pH(), ctx);
	}
};
