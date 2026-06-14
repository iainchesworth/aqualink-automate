#include <boost/test/unit_test.hpp>

#include "jandy/utility/spa_switch_assignment.h"

using namespace AqualinkAutomate::Utility;

//=============================================================================
// Spa-switch config row parser. The SAME "<switch>:<button> <function>" shape is
// emitted by both controllers, so one parser serves iAQ (tab-separated, from the
// 0x26 TableMessage) and OneTouch (space-padded screen lines) -- the basis of the
// feature working for either controller (open-source generality).
//=============================================================================

BOOST_AUTO_TEST_SUITE(SpaSwitchAssignment_TestSuite)

BOOST_AUTO_TEST_CASE(Parses_iAQ_TabSeparated_Row)
{
	// iAQ IAQ_TableMessage line: "1:2\tPool Light".
	auto r = ParseSpaSwitchAssignmentLine("1:2\tPool Light");
	BOOST_REQUIRE(r.has_value());
	BOOST_CHECK_EQUAL(static_cast<int>(r->switch_number), 1);
	BOOST_CHECK_EQUAL(static_cast<int>(r->button_number), 2);
	BOOST_CHECK_EQUAL(r->function, "Pool Light");
}

BOOST_AUTO_TEST_CASE(Parses_iAQ_SanitisedTab_Row)
{
	// As the iAQ row actually arrives after the message layer sanitises the tab separator
	// and trailing NUL to '?': "1:1?Spa Jets?".
	auto r = ParseSpaSwitchAssignmentLine("1:1?Spa Jets?");
	BOOST_REQUIRE(r.has_value());
	BOOST_CHECK_EQUAL(static_cast<int>(r->switch_number), 1);
	BOOST_CHECK_EQUAL(static_cast<int>(r->button_number), 1);
	BOOST_CHECK_EQUAL(r->function, "Spa Jets");
}

BOOST_AUTO_TEST_CASE(Parses_OneTouch_SpacePadded_Row)
{
	// OneTouch screen line: "1:2   Pool Light" (space-padded for the fixed-width display).
	auto r = ParseSpaSwitchAssignmentLine("1:2   Pool Light");
	BOOST_REQUIRE(r.has_value());
	BOOST_CHECK_EQUAL(static_cast<int>(r->switch_number), 1);
	BOOST_CHECK_EQUAL(static_cast<int>(r->button_number), 2);
	BOOST_CHECK_EQUAL(r->function, "Pool Light");
}

BOOST_AUTO_TEST_CASE(Parses_SecondSwitch_And_MultiWordFunction)
{
	auto r = ParseSpaSwitchAssignmentLine("2:4     Air Blower");
	BOOST_REQUIRE(r.has_value());
	BOOST_CHECK_EQUAL(static_cast<int>(r->switch_number), 2);
	BOOST_CHECK_EQUAL(static_cast<int>(r->button_number), 4);
	BOOST_CHECK_EQUAL(r->function, "Air Blower");
}

BOOST_AUTO_TEST_CASE(Trims_Surrounding_Whitespace)
{
	auto r = ParseSpaSwitchAssignmentLine("   1:1     Spa Jets   ");
	BOOST_REQUIRE(r.has_value());
	BOOST_CHECK_EQUAL(static_cast<int>(r->switch_number), 1);
	BOOST_CHECK_EQUAL(static_cast<int>(r->button_number), 1);
	BOOST_CHECK_EQUAL(r->function, "Spa Jets");
}

BOOST_AUTO_TEST_CASE(Rejects_NonAssignmentLines)
{
	// Function-picker entries, headers, blanks, and malformed rows are NOT assignments.
	BOOST_CHECK(!ParseSpaSwitchAssignmentLine("Spa Jets").has_value());        // picker entry
	BOOST_CHECK(!ParseSpaSwitchAssignmentLine("Devices").has_value());         // group header
	BOOST_CHECK(!ParseSpaSwitchAssignmentLine("   Spa Switch").has_value());   // page title
	BOOST_CHECK(!ParseSpaSwitchAssignmentLine("").has_value());                // blank
	BOOST_CHECK(!ParseSpaSwitchAssignmentLine("1").has_value());               // no colon
	BOOST_CHECK(!ParseSpaSwitchAssignmentLine("1:").has_value());              // no button
	BOOST_CHECK(!ParseSpaSwitchAssignmentLine("1:2").has_value());             // no function
	BOOST_CHECK(!ParseSpaSwitchAssignmentLine("1:2   ").has_value());          // empty function
	BOOST_CHECK(!ParseSpaSwitchAssignmentLine(":2 Pool").has_value());         // no switch
}

BOOST_AUTO_TEST_SUITE_END()
