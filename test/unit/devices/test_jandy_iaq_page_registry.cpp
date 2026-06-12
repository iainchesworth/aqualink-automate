#include <algorithm>
#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/iaq/iaq_page_registry.h"

using namespace AqualinkAutomate::Devices::IAQ;

BOOST_AUTO_TEST_SUITE(Jandy_IAQ_PageRegistry_TestSuite)

// =============================================================================
// The default registry -- the data pages the AqualinkTouch visits on start-up
// =============================================================================

BOOST_AUTO_TEST_CASE(DefaultRegistry_VisitsHeatSetpointPages)
{
	auto registry = DefaultAqualinkTouchRegistry();

	BOOST_REQUIRE_EQUAL(registry.size(), 2u);
	BOOST_CHECK_EQUAL(registry[0].label, "Pool Heat");
	BOOST_REQUIRE_EQUAL(registry[0].button_path.size(), 1u);
	BOOST_CHECK_EQUAL(registry[0].button_path[0], 0x02);   // home button 2 = Pool Heat
	BOOST_CHECK_EQUAL(registry[1].label, "Spa Heat");
	BOOST_CHECK_EQUAL(registry[1].button_path[0], 0x03);   // home button 3 = Spa Heat
}

// =============================================================================
// BuildSurveyCommandSequence -- flatten the registry into poll-ACK commands
// =============================================================================

BOOST_AUTO_TEST_CASE(BuildSequence_SingleTarget_OpensDwellsBacksDwells)
{
	PageRegistry registry{ { "Pool Heat", { 0x02 }, "setpoint" } };

	auto commands = BuildSurveyCommandSequence(registry, /*dwell_polls=*/2);

	// open (0x11+2=0x13), dwell x2 (0x00), back x1 (0x02), trailing dwell x(2/2+1=2) (0x00).
	const std::vector<std::uint8_t> expected{ 0x13, 0x00, 0x00, 0x02, 0x00, 0x00 };
	BOOST_CHECK(commands == expected);
}

BOOST_AUTO_TEST_CASE(BuildSequence_TwoLevelPath_BacksOncePerDescendedLevel)
{
	PageRegistry registry{ { "Deep", { 0x01, 0x04 }, "x" } };

	auto commands = BuildSurveyCommandSequence(registry, /*dwell_polls=*/0);

	// open: 0x12, 0x15 ; no in-dwell ; back x2: 0x02, 0x02 ; trailing dwell x(0/2+1=1): 0x00.
	const std::vector<std::uint8_t> expected{ 0x12, 0x15, 0x02, 0x02, 0x00 };
	BOOST_CHECK(commands == expected);
}

BOOST_AUTO_TEST_CASE(BuildSequence_DefaultRegistry_VisitsEveryTarget)
{
	auto commands = BuildSurveyCommandSequence(DefaultAqualinkTouchRegistry(), /*dwell_polls=*/4);

	// Two single-level targets: each = 1 open + 4 dwell + 1 back + 3 trailing dwell = 9 cmds.
	BOOST_CHECK_EQUAL(commands.size(), 18u);
	// First command opens Pool Heat (button 2 -> 0x13), and the Spa Heat open (button 3 -> 0x14)
	// appears once we have navigated back home.
	BOOST_CHECK_EQUAL(commands.front(), 0x13);
	BOOST_CHECK(std::find(commands.begin(), commands.end(), static_cast<std::uint8_t>(0x14)) != commands.end());
}

BOOST_AUTO_TEST_CASE(BuildSequence_EmptyRegistry_EmptySequence)
{
	BOOST_CHECK(BuildSurveyCommandSequence(PageRegistry{}).empty());
}

BOOST_AUTO_TEST_SUITE_END()
