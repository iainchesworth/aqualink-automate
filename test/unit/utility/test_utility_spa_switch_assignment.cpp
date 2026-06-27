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

BOOST_AUTO_TEST_CASE(Rejects_OverlongDigitRuns_WithoutThrowing)
{
	// Regression (remote DoS): the switch/button digit runs are unbounded, so an
	// attacker-supplied IAQ TableMessage line with a digit run wider than INT_MAX
	// (e.g. "99999999999") used to reach std::stoi and throw std::out_of_range.
	// On the serial dispatch path that throw terminated the whole daemon.  The
	// parser must now treat an out-of-range value as "not an assignment line" and
	// return std::nullopt rather than throw.
	BOOST_CHECK_NO_THROW((void)ParseSpaSwitchAssignmentLine("99999999999:1 Pool Light"));
	BOOST_CHECK(!ParseSpaSwitchAssignmentLine("99999999999:1 Pool Light").has_value());
	BOOST_CHECK(!ParseSpaSwitchAssignmentLine("1:99999999999 Pool Light").has_value());
	BOOST_CHECK(!ParseSpaSwitchAssignmentLine("999999999999999999999999:1 Pool Light").has_value());

	// Values that parse but fall outside the valid 1..255 range are still rejected.
	BOOST_CHECK(!ParseSpaSwitchAssignmentLine("0:1 Pool Light").has_value());
	BOOST_CHECK(!ParseSpaSwitchAssignmentLine("256:1 Pool Light").has_value());
	BOOST_CHECK(!ParseSpaSwitchAssignmentLine("1:256 Pool Light").has_value());

	// The boundary values remain accepted.
	auto hi = ParseSpaSwitchAssignmentLine("255:255 Pool Light");
	BOOST_REQUIRE(hi.has_value());
	BOOST_CHECK_EQUAL(static_cast<int>(hi->switch_number), 255);
	BOOST_CHECK_EQUAL(static_cast<int>(hi->button_number), 255);
}

BOOST_AUTO_TEST_SUITE_END()
