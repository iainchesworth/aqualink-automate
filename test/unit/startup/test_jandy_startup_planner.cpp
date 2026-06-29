#include <algorithm>
#include <cstdint>
#include <optional>
#include <ranges>
#include <set>

#include <boost/test/unit_test.hpp>

#include "jandy/startup/jandy_revision_capabilities.h"
#include "jandy/startup/jandy_startup_planner.h"
#include "jandy/startup/jandy_startup_types.h"

using namespace AqualinkAutomate::Jandy::Startup;
using DeviceType = AqualinkAutomate::Devices::JandyEmulatedDeviceTypes;

namespace
{
	const PlannedDevice* FindDevice(const StartupPlan& plan, DeviceType type)
	{
		for (const auto& device : plan.devices)
		{
			if (device.type == type)
			{
				return &device;
			}
		}
		return nullptr;
	}
}

BOOST_AUTO_TEST_SUITE(Jandy_Startup_Planner_TestSuite)

// =============================================================================
// DeriveProfile -- capabilities come from the master's discovery probe set
// =============================================================================

BOOST_AUTO_TEST_CASE(DeriveProfile_AqualinkTouchRange_SetsFlag)
{
	auto profile = StartupPlanner::DeriveProfile({ 0x30, 0x31, 0x32, 0x33 }, "", "");
	BOOST_CHECK(profile.probes_aqualinktouch);
	BOOST_CHECK(!profile.probes_onetouch);
	BOOST_CHECK(!profile.probes_pda);
}

BOOST_AUTO_TEST_CASE(DeriveProfile_OneTouchRange_SetsFlag)
{
	auto profile = StartupPlanner::DeriveProfile({ 0x40, 0x41 }, "", "");
	BOOST_CHECK(profile.probes_onetouch);
	BOOST_CHECK(!profile.probes_aqualinktouch);
}

BOOST_AUTO_TEST_CASE(DeriveProfile_Iaqualink2Slot_OnlyWhenA3Probed)
{
	// The verified fact: the master probes 0xa3 but never 0xa0/a1/a2.
	BOOST_CHECK(StartupPlanner::DeriveProfile({ 0xA3 }, "", "").has_iaqualink2_slot);
	BOOST_CHECK(!StartupPlanner::DeriveProfile({ 0xA1 }, "", "").has_iaqualink2_slot);
}

BOOST_AUTO_TEST_CASE(DeriveProfile_CarriesIdentity)
{
	auto profile = StartupPlanner::DeriveProfile({ 0x33 }, "PD-8 Combo", "T.0.1");
	BOOST_CHECK_EQUAL(profile.model, "PD-8 Combo");
	BOOST_CHECK_EQUAL(profile.revision, "T.0.1");
}

// =============================================================================
// Plan -- data-gathering method + device choice
// =============================================================================

BOOST_AUTO_TEST_CASE(Plan_AqualinkTouch_PrefersPagePushOverSpider)
{
	PanelProfile profile;
	profile.probes_aqualinktouch = true;
	profile.probes_onetouch = true;   // both probed -- page-push must win

	auto plan = StartupPlanner::Plan(profile);

	BOOST_CHECK(plan.method == DataGatheringMethod::PagePush);
	BOOST_REQUIRE(FindDevice(plan, DeviceType::IAQ) != nullptr);
	BOOST_CHECK_EQUAL(FindDevice(plan, DeviceType::IAQ)->candidate_ids.front(), 0x33);
	BOOST_CHECK(FindDevice(plan, DeviceType::SerialAdapter) != nullptr);  // always present
	BOOST_CHECK(FindDevice(plan, DeviceType::OneTouch) == nullptr);       // not when page-push wins
}

BOOST_AUTO_TEST_CASE(Plan_OneTouchOnly_FallsBackToSpider)
{
	PanelProfile profile;
	profile.probes_onetouch = true;

	auto plan = StartupPlanner::Plan(profile);

	BOOST_CHECK(plan.method == DataGatheringMethod::MenuSpider);
	BOOST_CHECK(FindDevice(plan, DeviceType::OneTouch) != nullptr);
	BOOST_CHECK(FindDevice(plan, DeviceType::IAQ) == nullptr);
}

BOOST_AUTO_TEST_CASE(Plan_PdaOnly_UsesPdaGraph)
{
	PanelProfile profile;
	profile.probes_pda = true;

	auto plan = StartupPlanner::Plan(profile);

	BOOST_CHECK(plan.method == DataGatheringMethod::PdaGraph);
	BOOST_CHECK(FindDevice(plan, DeviceType::PDA) != nullptr);
}

BOOST_AUTO_TEST_CASE(Plan_NoController_ObserveOnly_StillEmulatesSerialAdapter)
{
	PanelProfile profile;  // nothing probed

	auto plan = StartupPlanner::Plan(profile);

	BOOST_CHECK(plan.method == DataGatheringMethod::ObserveOnly);
	BOOST_CHECK(FindDevice(plan, DeviceType::SerialAdapter) != nullptr);
	BOOST_CHECK_EQUAL(plan.devices.size(), 1u);  // SerialAdapter only
}

// =============================================================================
// Address selection -- presence-aware placement (never collide with real devices)
// =============================================================================

BOOST_AUTO_TEST_CASE(ResolveAddresses_OneTouch_NeverUsesReservedUnit4Slot)
{
	// Units 1-3 (0x41, 0x40, 0x42) are all occupied by real OneTouches. Unit 4 (0x43) is the
	// AquaLink-PC-reserved control-panel-4 slot, so the emulated OneTouch must stay UNRESOLVED
	// rather than land on 0x43 (Table 1 note **).
	PanelProfile profile;
	profile.probes_onetouch = true;
	auto plan = StartupPlanner::Plan(profile);

	StartupPlanner::ResolveAddresses(plan, { 0x41, 0x40, 0x42 });

	const auto* onetouch = FindDevice(plan, DeviceType::OneTouch);
	BOOST_REQUIRE(onetouch != nullptr);
	BOOST_CHECK(!onetouch->resolved);
	BOOST_CHECK_EQUAL(onetouch->selected_id, 0x00);
	// 0x43 must not even be offered as a candidate.
	BOOST_CHECK(std::ranges::find(onetouch->candidate_ids, 0x43) == onetouch->candidate_ids.cend());
}

BOOST_AUTO_TEST_CASE(SelectFreeAddress_PrefersFirstFree)
{
	BOOST_CHECK(StartupPlanner::SelectFreeAddress({ 0x33, 0x32, 0x31, 0x30 }, {}) == std::optional<std::uint8_t>(0x33));
	BOOST_CHECK(StartupPlanner::SelectFreeAddress({ 0x33, 0x32, 0x31, 0x30 }, { 0x33 }) == std::optional<std::uint8_t>(0x32));
	BOOST_CHECK(StartupPlanner::SelectFreeAddress({ 0x33, 0x32, 0x31, 0x30 }, { 0x33, 0x32, 0x31, 0x30 }) == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ResolveAddresses_FreeBus_UsesPreferred)
{
	PanelProfile profile;
	profile.probes_aqualinktouch = true;
	auto plan = StartupPlanner::Plan(profile);

	StartupPlanner::ResolveAddresses(plan, {});

	const auto* iaq = FindDevice(plan, DeviceType::IAQ);
	BOOST_REQUIRE(iaq != nullptr);
	BOOST_CHECK(iaq->resolved);
	BOOST_CHECK_EQUAL(iaq->selected_id, 0x33);
}

BOOST_AUTO_TEST_CASE(ResolveAddresses_RealTouchAt0x33_RelocatesToSecondSlot)
{
	// A real AqualinkTouch already answers at 0x33 -> our emulated one stands up at 0x32.
	PanelProfile profile;
	profile.probes_aqualinktouch = true;
	auto plan = StartupPlanner::Plan(profile);

	StartupPlanner::ResolveAddresses(plan, { 0x33 });

	const auto* iaq = FindDevice(plan, DeviceType::IAQ);
	BOOST_REQUIRE(iaq != nullptr);
	BOOST_CHECK(iaq->resolved);
	BOOST_CHECK_EQUAL(iaq->selected_id, 0x32);
}

BOOST_AUTO_TEST_CASE(ResolveAddresses_AllSlotsTaken_LeavesUnresolved)
{
	PanelProfile profile;
	profile.probes_aqualinktouch = true;
	auto plan = StartupPlanner::Plan(profile);

	StartupPlanner::ResolveAddresses(plan, { 0x30, 0x31, 0x32, 0x33 });

	const auto* iaq = FindDevice(plan, DeviceType::IAQ);
	BOOST_REQUIRE(iaq != nullptr);
	BOOST_CHECK(!iaq->resolved);
	BOOST_CHECK_EQUAL(iaq->selected_id, 0x00);
}

// =============================================================================
// Revision integration -- firmware revision as an early/fallback capability signal
// =============================================================================

BOOST_AUTO_TEST_CASE(DeriveProfile_PopulatesRevisionCaps)
{
	auto profile = StartupPlanner::DeriveProfile({ 0x33 }, "PD-8 Combo", "REV T.0.1");
	BOOST_CHECK(profile.revision_caps.is_known);
	BOOST_CHECK(profile.revision_caps.aqualink_touch);
	BOOST_CHECK(profile.revision_caps.variable_speed_pumps);
}

BOOST_AUTO_TEST_CASE(Plan_NoProbesButTouchCapableRevision_ProvisionalPagePush)
{
	// Revision sourced (Rev T) before the master's probe cycle is seen -> classify as page-push.
	PanelProfile profile;
	profile.revision_caps = DeriveRevisionCapabilities("REV T.0.1");

	auto plan = StartupPlanner::Plan(profile);

	BOOST_CHECK(plan.method == DataGatheringMethod::PagePush);
	BOOST_CHECK(FindDevice(plan, DeviceType::IAQ) != nullptr);
}

BOOST_AUTO_TEST_CASE(Plan_TouchProbedButPpdRevision_SkipsAndFlagsInconsistent)
{
	// The master probes the touch range, but the revision (Rev M) predates AqualinkTouch (Rev Q):
	// warn + skip -- we do NOT stand up the page-push emulation, falling back to passive decode.
	// The SerialAdapter survives (Rev M >= Rev I). The contradiction is still flagged.
	PanelProfile profile;
	profile.probes_aqualinktouch = true;
	profile.revision_caps = DeriveRevisionCapabilities("M");

	auto plan = StartupPlanner::Plan(profile);

	BOOST_CHECK(plan.method == DataGatheringMethod::ObserveOnly);
	BOOST_CHECK(FindDevice(plan, DeviceType::IAQ) == nullptr);
	BOOST_CHECK(FindDevice(plan, DeviceType::SerialAdapter) != nullptr);
	BOOST_CHECK(!plan.revision_consistent);
}

BOOST_AUTO_TEST_CASE(Plan_OneTouchProbedButPreRevI_SkipsAndFlagsInconsistent)
{
	// OneTouch AND the Serial Adapter both first appear at Rev I (2000); a Rev H panel probing the
	// OneTouch range is a contradiction. Warn + skip: neither is emulated, leaving passive decode.
	PanelProfile profile;
	profile.probes_onetouch = true;
	profile.revision_caps = DeriveRevisionCapabilities("H");

	auto plan = StartupPlanner::Plan(profile);

	BOOST_CHECK(plan.method == DataGatheringMethod::ObserveOnly);
	BOOST_CHECK(FindDevice(plan, DeviceType::OneTouch) == nullptr);
	BOOST_CHECK(FindDevice(plan, DeviceType::SerialAdapter) == nullptr);
	BOOST_CHECK(plan.devices.empty());
	BOOST_CHECK(!plan.revision_consistent);
}

// =============================================================================
// Revision gate -- refuse to emulate a device below its Table 1 minimum revision
// =============================================================================

BOOST_AUTO_TEST_CASE(Plan_SerialAdapterGatedBelowRevI)
{
	// Rev H predates Serial Adapter support (Rev I); with nothing else probed the plan is empty.
	PanelProfile profile;
	profile.revision_caps = DeriveRevisionCapabilities("H");

	auto plan = StartupPlanner::Plan(profile);

	BOOST_CHECK(FindDevice(plan, DeviceType::SerialAdapter) == nullptr);
	BOOST_CHECK(plan.method == DataGatheringMethod::ObserveOnly);
	BOOST_CHECK(plan.devices.empty());
}

BOOST_AUTO_TEST_CASE(Plan_SerialAdapterPresentAtRevI)
{
	// Rev I is exactly the minimum -- the Serial Adapter stands up.
	PanelProfile profile;
	profile.revision_caps = DeriveRevisionCapabilities("I");

	auto plan = StartupPlanner::Plan(profile);

	BOOST_CHECK(FindDevice(plan, DeviceType::SerialAdapter) != nullptr);
}

BOOST_AUTO_TEST_CASE(Plan_UnknownRevisionDoesNotGate)
{
	// No revision sourced yet: the gate must not fire (we lean on the observed probe set), so the
	// OneTouch emulation still stands up.
	PanelProfile profile;
	profile.probes_onetouch = true;   // revision_caps left default -> is_known == false

	auto plan = StartupPlanner::Plan(profile);

	BOOST_CHECK(plan.method == DataGatheringMethod::MenuSpider);
	BOOST_CHECK(FindDevice(plan, DeviceType::OneTouch) != nullptr);
	BOOST_CHECK(FindDevice(plan, DeviceType::SerialAdapter) != nullptr);
}

BOOST_AUTO_TEST_CASE(Plan_TouchProbedAndTouchCapableRevision_Consistent)
{
	PanelProfile profile;
	profile.probes_aqualinktouch = true;
	profile.revision_caps = DeriveRevisionCapabilities("T");

	auto plan = StartupPlanner::Plan(profile);

	BOOST_CHECK(plan.revision_consistent);
	BOOST_CHECK(plan.revision_caps.aqualink_touch);
}

BOOST_AUTO_TEST_CASE(Plan_TouchCapableRevButNoTouchProbed_IsNotInconsistent)
{
	// Rev T but only OneTouch probed: panel is touch-capable yet a OneTouch UI is installed --
	// normal, NOT a contradiction. Method follows the live probe (spider).
	PanelProfile profile;
	profile.probes_onetouch = true;
	profile.revision_caps = DeriveRevisionCapabilities("T");

	auto plan = StartupPlanner::Plan(profile);

	BOOST_CHECK(plan.method == DataGatheringMethod::MenuSpider);
	BOOST_CHECK(plan.revision_consistent);
}

BOOST_AUTO_TEST_CASE(Plan_NoControllerAndPpdRevision_ObserveOnly)
{
	PanelProfile profile;
	profile.revision_caps = DeriveRevisionCapabilities("M");  // PPD, no touch, nothing probed

	auto plan = StartupPlanner::Plan(profile);

	BOOST_CHECK(plan.method == DataGatheringMethod::ObserveOnly);
}

BOOST_AUTO_TEST_SUITE_END()
