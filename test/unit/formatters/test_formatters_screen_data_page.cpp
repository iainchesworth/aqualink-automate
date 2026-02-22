#include <format>
#include <string>

#include <boost/test/unit_test.hpp>

#include "jandy/formatters/screen_data_page_formatter.h"
#include "jandy/utility/screen_data_page.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Utility;

BOOST_AUTO_TEST_SUITE(Formatters_ScreenDataPageTestSuite)

// =============================================================================
// Empty Page
// =============================================================================

BOOST_AUTO_TEST_CASE(TestFormat_EmptyPage_HasHeaderAndFooter)
{
	ScreenDataPage page(4);
	auto result = std::format("{}", page);

	// Should contain header and footer borders
	BOOST_CHECK(result.find("+------------------+") != std::string::npos);

	// Count the borders - should have exactly 2 (header + footer)
	size_t count = 0;
	size_t pos = 0;
	while ((pos = result.find("+------------------+", pos)) != std::string::npos)
	{
		++count;
		pos += 1;
	}
	BOOST_CHECK_EQUAL(count, static_cast<size_t>(2));
}

BOOST_AUTO_TEST_CASE(TestFormat_EmptyPage_HasBlankRows)
{
	ScreenDataPage page(4);
	auto result = std::format("{}", page);

	// Should contain row markers
	BOOST_CHECK(result.find("|") != std::string::npos);
}

// =============================================================================
// Content Rows
// =============================================================================

BOOST_AUTO_TEST_CASE(TestFormat_ContentRow_CenteredText)
{
	ScreenDataPage page(4);
	page[0].Text = "Equipment ON/OFF";

	auto result = std::format("{}", page);

	// Should contain the text within the box format
	BOOST_CHECK(result.find("Equipment ON/OFF") != std::string::npos);
	BOOST_CHECK(result.find("|") != std::string::npos);
}

// =============================================================================
// Highlighted Row
// =============================================================================

BOOST_AUTO_TEST_CASE(TestFormat_HighlightedRow_ShowsArrow)
{
	ScreenDataPage page(4);
	page[1].Text = "Filter Pump";
	page[1].HighlightState = ScreenDataPage::HighlightStates::Highlighted;

	auto result = std::format("{}", page);

	// Highlighted row should have <-- marker
	BOOST_CHECK(result.find("<--") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestFormat_NonHighlightedRow_NoArrow)
{
	ScreenDataPage page(4);
	page[0].Text = "Equipment ON/OFF";
	page[0].HighlightState = ScreenDataPage::HighlightStates::Normal;

	// Format a single-row page to check no arrow
	ScreenDataPage single_page(1);
	single_page[0].Text = "Test";
	single_page[0].HighlightState = ScreenDataPage::HighlightStates::Normal;

	auto result = std::format("{}", single_page);

	// Non-highlighted row should have empty arrow
	BOOST_CHECK(result.find("<--") == std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestFormat_MixedHighlightedAndNormal)
{
	ScreenDataPage page(3);
	page[0].Text = "Title";
	page[0].HighlightState = ScreenDataPage::HighlightStates::Normal;
	page[1].Text = "Selected";
	page[1].HighlightState = ScreenDataPage::HighlightStates::Highlighted;
	page[2].Text = "Other";
	page[2].HighlightState = ScreenDataPage::HighlightStates::Normal;

	auto result = std::format("{}", page);

	// Only one <-- marker should be present
	size_t count = 0;
	size_t pos = 0;
	while ((pos = result.find("<--", pos)) != std::string::npos)
	{
		++count;
		pos += 3;
	}
	BOOST_CHECK_EQUAL(count, static_cast<size_t>(1));
}

// =============================================================================
// ShiftLines edge cases (Bug 10 regression)
// =============================================================================

BOOST_AUTO_TEST_CASE(TestShiftLines_ZeroRows_DoesNotCrash)
{
	// Regression: m_Rows.size() - 2 underflows when size < 2
	ScreenDataPage page(0);
	BOOST_CHECK_NO_THROW(page.ShiftLines(ScreenDataPage::ShiftDirections::Up, 0, 0, 1));
}

BOOST_AUTO_TEST_CASE(TestShiftLines_OneRow_DoesNotCrash)
{
	ScreenDataPage page(1);
	BOOST_CHECK_NO_THROW(page.ShiftLines(ScreenDataPage::ShiftDirections::Up, 0, 0, 1));
}

BOOST_AUTO_TEST_SUITE_END()
