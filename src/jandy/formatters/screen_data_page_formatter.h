#pragma once

#include <format>
#include <string>
#include <string_view>

#include "jandy/utility/screen_data_page.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

template<>
struct std::formatter<AqualinkAutomate::Utility::ScreenDataPage> : std::formatter<std::string>
{
	template<typename FormatContext>
	auto format(const AqualinkAutomate::Utility::ScreenDataPage& page, FormatContext& ctx) const
	{
		using AqualinkAutomate::Utility::ScreenDataPage::HighlightStates::Highlighted;

		// +------------------+
		// | Equipment Status |
		// |                  |
		// |                  |
		// |                  |
		// |                  |
		// |                  |
		// |                  |
		// |                  |
		// |                  |
		// |                  |
		// |                  |
		// |                  |
		// +------------------+

		static const std::string_view HEADER_FOOTER{ "+------------------+\n" };
		static const std::string_view PAGE_ROW{ "| {:^16} | {}\n" };
		
		auto ctx_it = ctx.out();

		std::copy(HEADER_FOOTER.begin(), HEADER_FOOTER.end(), ctx_it);

		for (const auto& row : page.m_Rows)
		{
			const auto highlight_arrow = (Highlighted == row.HighlightState) ? "<--" : "";
			std::vformat_to(ctx_it, PAGE_ROW, std::make_format_args(row.Text, highlight_arrow));
		}

		std::copy(HEADER_FOOTER.begin(), HEADER_FOOTER.end(), ctx_it);

		return ctx_it;
	}
};
