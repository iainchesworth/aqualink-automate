#include <boost/test/unit_test.hpp>

#include "jandy/utility/screen_data_page.h"
#include "jandy/utility/screen_data_page_updater.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Utility;
using namespace AqualinkAutomate::Utility::ScreenDataPageUpdaterImpl;

BOOST_AUTO_TEST_SUITE(ScreenDataPageUpdater_TestSuite)

namespace
{
	ScreenDataPage MakePopulatedPage()
	{
		ScreenDataPage page(5);
		page[0].Text = "A";
		page[1].Text = "B";
		page[2].Text = "C";
		page[3].Text = "D";
		page[4].Text = "E";
		return page;
	}
}

// A valid shift fed through the state machine behaves as ScreenDataPage::ShiftLines does.
BOOST_AUTO_TEST_CASE(TestUpdaterShiftValid)
{
	ScreenDataPage page = MakePopulatedPage();

	ScreenDataPageUpdater<ScreenDataPage> updater(page);
	updater.initiate();

	BOOST_CHECK_NO_THROW(updater.process_event(evShift(ScreenDataPage::ShiftDirections::Up, 1, 3, 1)));

	BOOST_CHECK_EQUAL(page[0].Text, "A");
	BOOST_CHECK_EQUAL(page[1].Text, "C");
	BOOST_CHECK_EQUAL(page[2].Text, "D");
	BOOST_CHECK_EQUAL(page[3].Text, "");
	BOOST_CHECK_EQUAL(page[4].Text, "E");
}

// Regression: an oversized shift driven through evShift (e.g. a malformed/hostile wire frame)
// must not corrupt or read/write out of bounds; it is rejected and leaves the page untouched.
BOOST_AUTO_TEST_CASE(TestUpdaterOversizedShiftIsSafe)
{
	ScreenDataPage page = MakePopulatedPage();

	ScreenDataPageUpdater<ScreenDataPage> updater(page);
	updater.initiate();

	// span = lines 1..3 (3 rows); a shift of 250 vastly exceeds it.
	BOOST_CHECK_NO_THROW(updater.process_event(evShift(ScreenDataPage::ShiftDirections::Up, 1, 3, 250)));

	BOOST_CHECK_EQUAL(page[0].Text, "A");
	BOOST_CHECK_EQUAL(page[1].Text, "B");
	BOOST_CHECK_EQUAL(page[2].Text, "C");
	BOOST_CHECK_EQUAL(page[3].Text, "D");
	BOOST_CHECK_EQUAL(page[4].Text, "E");

	// And in the Down direction (negative rotate offset).
	BOOST_CHECK_NO_THROW(updater.process_event(evShift(ScreenDataPage::ShiftDirections::Down, 1, 3, 250)));

	BOOST_CHECK_EQUAL(page[0].Text, "A");
	BOOST_CHECK_EQUAL(page[1].Text, "B");
	BOOST_CHECK_EQUAL(page[2].Text, "C");
	BOOST_CHECK_EQUAL(page[3].Text, "D");
	BOOST_CHECK_EQUAL(page[4].Text, "E");
}

BOOST_AUTO_TEST_SUITE_END()
