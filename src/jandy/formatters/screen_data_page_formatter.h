#pragma once

#include <format>
#include <string>

#include "jandy/utility/screen_data_page.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

template<>
struct std::formatter<AqualinkAutomate::Utility::ScreenDataPage>
{
	template <typename FormatParseContext>
	auto parse(FormatParseContext& ctx)
	{
		using AqualinkAutomate::Utility::ScreenDataPage;

		return ctx.end();
	}

	template<typename FormatContext>
	auto format(const AqualinkAutomate::Utility::ScreenDataPage& s, FormatContext& ctx)
	{
		using AqualinkAutomate::Utility::ScreenDataPage;

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

		ctx.out() = std::format_to(ctx.out(), "+------------------+\n");

		for (const auto& row : s.m_Rows)
		{
			ctx.out() = std::format_to(
				ctx.out(), 
				"| {:^16} | {}\n", 
				row.Text, 
				(ScreenDataPage::HighlightStates::Highlighted == row.HighlightState) ? "<--" : ""
			);
		}

		ctx.out() = std::format_to(ctx.out(), "+------------------+\n");

		return ctx.out();
	}
};
