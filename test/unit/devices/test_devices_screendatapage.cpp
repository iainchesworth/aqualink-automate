#include <boost/test/unit_test.hpp>

#include "jandy/utility/screen_data_page.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Utility;

BOOST_AUTO_TEST_SUITE(ScreenDataPage_TestSuite)

BOOST_AUTO_TEST_CASE(ConstructorTest)
{
    ScreenDataPage page(3);
    BOOST_CHECK_EQUAL(page.Size(), 3);

    BOOST_CHECK(page[0].Text.empty());
    BOOST_CHECK(ScreenDataPage::HighlightStates::Normal == page[0].HighlightState);
    BOOST_CHECK(!page[0].HighlightRange.has_value());

    BOOST_CHECK(page[1].Text.empty());
    BOOST_CHECK(ScreenDataPage::HighlightStates::Normal == page[1].HighlightState);
    BOOST_CHECK(!page[1].HighlightRange.has_value());

    BOOST_CHECK(page[2].Text.empty());
    BOOST_CHECK(ScreenDataPage::HighlightStates::Normal == page[2].HighlightState);
    BOOST_CHECK(!page[2].HighlightRange.has_value());
}

BOOST_AUTO_TEST_CASE(OperatorTest)
{
    ScreenDataPage page(1);

    auto& row = page[0];

    BOOST_CHECK(row.Text.empty());
    BOOST_CHECK(ScreenDataPage::HighlightStates::Normal == row.HighlightState);
    BOOST_CHECK(!row.HighlightRange.has_value());
}

BOOST_AUTO_TEST_CASE(OperatorOutOfRangeExceptionTest)
{
    ScreenDataPage page(1);
    BOOST_CHECK_THROW(page[3], std::out_of_range);
}

BOOST_AUTO_TEST_CASE(ClearTest)
{
    ScreenDataPage page(5);
    
    page[0].Text = "A";
    page[1].Text = "B";
    page[2].Text = "C";
    page[3].Text = "D";
    page[4].Text = "E";

    BOOST_REQUIRE("A" == page[0].Text);
    BOOST_REQUIRE("B" == page[1].Text);
    BOOST_REQUIRE("C" == page[2].Text);
    BOOST_REQUIRE("D" == page[3].Text);
    BOOST_REQUIRE("E" == page[4].Text);

    page.Clear();

    for (std::size_t i = 0; i < page.Size(); ++i)
    {
        auto& row = page[i];

        BOOST_CHECK(row.Text.empty());
        BOOST_CHECK(ScreenDataPage::HighlightStates::Normal == row.HighlightState);
        BOOST_CHECK(!row.HighlightRange.has_value());
    }
}

BOOST_AUTO_TEST_CASE(HighlightTest)
{
    ScreenDataPage page(5);
    
    page.Highlight(2);
    auto& row = page[2];

    BOOST_REQUIRE(ScreenDataPage::HighlightStates::Highlighted == row.HighlightState);

    page.Clear();

    BOOST_CHECK(ScreenDataPage::HighlightStates::Normal == row.HighlightState);
}

BOOST_AUTO_TEST_CASE(HighlightCharsTest)
{
    ScreenDataPage page(5);

    page.HighlightChars(2, 1, 3);
    auto& row = page[2];

    BOOST_CHECK(ScreenDataPage::HighlightStates::PartiallyHighlighted == row.HighlightState);
    BOOST_REQUIRE(row.HighlightRange.has_value());
    BOOST_CHECK_EQUAL(row.HighlightRange.value().Start, 1);
    BOOST_CHECK_EQUAL(row.HighlightRange.value().Stop, 3);

    page.Clear();

    BOOST_CHECK(ScreenDataPage::HighlightStates::Normal == row.HighlightState);
}

BOOST_AUTO_TEST_CASE(ShiftLinesTest)
{
    ScreenDataPage page(5);

    page[0].Text = "A";
    page[1].Text = "B";
    page[2].Text = "C";
    page[3].Text = "D";
    page[4].Text = "E";

    page.ShiftLines(ScreenDataPage::ShiftDirections::Up, 1, 3, 1);

    BOOST_CHECK_EQUAL(page[0].Text, "A");
    BOOST_CHECK_EQUAL(page[1].Text, "C");
    BOOST_CHECK_EQUAL(page[2].Text, "D");
    BOOST_CHECK_EQUAL(page[3].Text, "");
    BOOST_CHECK_EQUAL(page[4].Text, "E");

    page.ShiftLines(ScreenDataPage::ShiftDirections::Down, 2, 4, 2);

    BOOST_CHECK_EQUAL(page[0].Text, "A");
    BOOST_CHECK_EQUAL(page[1].Text, "C");
    BOOST_CHECK_EQUAL(page[2].Text, "");
    BOOST_CHECK_EQUAL(page[3].Text, "");
    BOOST_CHECK_EQUAL(page[4].Text, "D");
}

BOOST_AUTO_TEST_CASE(TestOutOfRangeLineId)
{
    ScreenDataPage page(5);

    BOOST_CHECK_NO_THROW(page.Highlight(10));
    BOOST_CHECK_NO_THROW(page.HighlightChars(10, 0, 5));
    BOOST_CHECK_NO_THROW(page.ShiftLines(ScreenDataPage::ShiftDirections::Up, 10, 11, 1));
}

// Regression: a shift larger than the [start, end] span pushed the std::rotate pivot
// (start + offset) and the subsequent clear range outside the span, causing an out-of-bounds
// std::rotate / std::for_each over m_Rows. The shift count must be clamped/rejected.
BOOST_AUTO_TEST_CASE(TestShiftLinesOversizedShiftIsRejected)
{
    ScreenDataPage page(5);

    page[0].Text = "A";
    page[1].Text = "B";
    page[2].Text = "C";
    page[3].Text = "D";
    page[4].Text = "E";

    // span = lines 1..3 (3 rows). A shift of 4 exceeds the span -> must be a safe no-op.
    BOOST_CHECK_NO_THROW(page.ShiftLines(ScreenDataPage::ShiftDirections::Up, 1, 3, 4));

    BOOST_CHECK_EQUAL(page[0].Text, "A");
    BOOST_CHECK_EQUAL(page[1].Text, "B");
    BOOST_CHECK_EQUAL(page[2].Text, "C");
    BOOST_CHECK_EQUAL(page[3].Text, "D");
    BOOST_CHECK_EQUAL(page[4].Text, "E");

    // Same for the Down direction (offset would go negative -> pivot before start).
    BOOST_CHECK_NO_THROW(page.ShiftLines(ScreenDataPage::ShiftDirections::Down, 1, 3, 4));

    BOOST_CHECK_EQUAL(page[0].Text, "A");
    BOOST_CHECK_EQUAL(page[1].Text, "B");
    BOOST_CHECK_EQUAL(page[2].Text, "C");
    BOOST_CHECK_EQUAL(page[3].Text, "D");
    BOOST_CHECK_EQUAL(page[4].Text, "E");
}

// A shift exactly equal to the span size is the maximum legal shift and must still be a no-op
// in effect (rotating a range by its full length leaves it unchanged) without going OOB.
BOOST_AUTO_TEST_CASE(TestShiftLinesFullSpanShiftIsSafe)
{
    ScreenDataPage page(5);

    page[0].Text = "A";
    page[1].Text = "B";
    page[2].Text = "C";
    page[3].Text = "D";
    page[4].Text = "E";

    // span = lines 1..3 (3 rows); shift of 3 == span size: the whole span is cleared.
    BOOST_CHECK_NO_THROW(page.ShiftLines(ScreenDataPage::ShiftDirections::Up, 1, 3, 3));

    BOOST_CHECK_EQUAL(page[0].Text, "A");
    BOOST_CHECK_EQUAL(page[1].Text, "");
    BOOST_CHECK_EQUAL(page[2].Text, "");
    BOOST_CHECK_EQUAL(page[3].Text, "");
    BOOST_CHECK_EQUAL(page[4].Text, "E");
}

// A zero-line shift is a no-op (and previously still ran the rotate machinery).
BOOST_AUTO_TEST_CASE(TestShiftLinesZeroShiftIsNoOp)
{
    ScreenDataPage page(5);

    page[0].Text = "A";
    page[1].Text = "B";
    page[2].Text = "C";
    page[3].Text = "D";
    page[4].Text = "E";

    BOOST_CHECK_NO_THROW(page.ShiftLines(ScreenDataPage::ShiftDirections::Up, 1, 3, 0));

    BOOST_CHECK_EQUAL(page[0].Text, "A");
    BOOST_CHECK_EQUAL(page[1].Text, "B");
    BOOST_CHECK_EQUAL(page[2].Text, "C");
    BOOST_CHECK_EQUAL(page[3].Text, "D");
    BOOST_CHECK_EQUAL(page[4].Text, "E");
}

BOOST_AUTO_TEST_SUITE_END()
