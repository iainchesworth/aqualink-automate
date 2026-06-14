#include <boost/test/unit_test.hpp>

#include "support/unit_test_onetouchdevice.h"

using namespace AqualinkAutomate;

//=============================================================================
// OneTouch spa-switch config decode: the "Spa Switch" menu page lists button
// assignments as "<switch>:<button>   <function>" screen lines. PageProcessor_SpaSwitch
// decodes them into the controller-agnostic DataHub map -- the OneTouch half of the
// same read path the iAQ provides (Phase 4b, open-source generality).
//=============================================================================

BOOST_FIXTURE_TEST_SUITE(OneTouchSpaSwitchConfig_TestSuite, Test::OneTouchDevice)

BOOST_AUTO_TEST_CASE(SpaSwitchPage_PopulatesAssignmentMap)
{
	InitialiseOneTouchDevice();

	// The real OneTouch "Spa Switch" page (title at line 0, assignments on the rows below).
	LoadAndSignalTestPage({
		{ 0, "   Spa Switch" },
		{ 3, "1:1     Spa Jets" },
		{ 4, "1:2   Pool Light" },
		{ 5, "1:3   Air Blower" },
		{ 6, "1:4     Spillway" },
		{ 7, "2:1     Swim Jet" },
		{ 8, "2:4   Pool Light" },
	});

	auto& hub = DataHub();
	BOOST_CHECK_EQUAL(hub.SpaSwitchAssignment(1, 1).value_or(""), "Spa Jets");
	BOOST_CHECK_EQUAL(hub.SpaSwitchAssignment(1, 2).value_or(""), "Pool Light");
	BOOST_CHECK_EQUAL(hub.SpaSwitchAssignment(1, 3).value_or(""), "Air Blower");
	BOOST_CHECK_EQUAL(hub.SpaSwitchAssignment(1, 4).value_or(""), "Spillway");
	BOOST_CHECK_EQUAL(hub.SpaSwitchAssignment(2, 1).value_or(""), "Swim Jet");
	BOOST_CHECK_EQUAL(hub.SpaSwitchAssignment(2, 4).value_or(""), "Pool Light");
}

BOOST_AUTO_TEST_CASE(NonSpaSwitchPage_DoesNotMisparseClock)
{
	// The home screen shows the time as "1:20 PM" -- which looks like "switch 1, button 20" to a
	// naive parser. Page gating (the "Spa Switch" detector) means a non-config page is never fed to
	// the assignment parser, so the clock must NOT create a bogus assignment.
	InitialiseOneTouchDevice();

	LoadAndSignalTestPage({
		{ 0, "Jandy AquaLinkRS" },
		{ 3, "    1:20 PM" },
		{ 5, "   Pool 22`C" },
	});

	BOOST_CHECK(DataHub().SpaSwitchAssignments().empty());
}

BOOST_AUTO_TEST_SUITE_END()
