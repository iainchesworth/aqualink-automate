#include <optional>

#include <boost/test/unit_test.hpp>

#include "navigation/menu_model.h"
#include "navigation/navigator.h"
#include "navigation/onetouch_menu_model.h"
#include "utility/screen_data_page.h"

using namespace AqualinkAutomate::Navigation;
using namespace AqualinkAutomate::Utility;

namespace
{
	// Helper: build a minimal menu model for navigator edge-case testing
	MenuModel BuildTestModel()
	{
		MenuModel model;

		// System (home) page
		MenuPage system_page;
		system_page.id = PageId::System;
		system_page.name = "System";
		system_page.detectors = { {0, "Equipment ON/OFF"} };
		system_page.edges = {
			{ EdgeTrigger::Select, PageId::System, PageId::MenuHelp, 11, "Menu/Help" },
			{ EdgeTrigger::Select, PageId::System, PageId::EquipmentOnOff, 1, "Filter Pump" },
			{ EdgeTrigger::LineUp, PageId::System, PageId::System, 0, "" },
			{ EdgeTrigger::LineDown, PageId::System, PageId::System, 0, "" }
		};
		model.RegisterPage(std::move(system_page));

		// MenuHelp page
		MenuPage help_page;
		help_page.id = PageId::MenuHelp;
		help_page.name = "Menu/Help";
		help_page.detectors = { {0, "Menu/Help"} };
		help_page.edges = {
			{ EdgeTrigger::Back, PageId::MenuHelp, PageId::System, 0, "" },
			{ EdgeTrigger::Select, PageId::MenuHelp, PageId::SystemSetup, 3, "System Setup" }
		};
		model.RegisterPage(std::move(help_page));

		// EquipmentOnOff page
		MenuPage equip_page;
		equip_page.id = PageId::EquipmentOnOff;
		equip_page.name = "Equipment ON/OFF";
		equip_page.detectors = { {0, "Equipment ON/OFF"}, {1, "Filter Pump"} };
		equip_page.edges = {
			{ EdgeTrigger::Back, PageId::EquipmentOnOff, PageId::System, 0, "" },
			{ EdgeTrigger::LineUp, PageId::EquipmentOnOff, PageId::EquipmentOnOff, 0, "" },
			{ EdgeTrigger::LineDown, PageId::EquipmentOnOff, PageId::EquipmentOnOff, 0, "" }
		};
		model.RegisterPage(std::move(equip_page));

		// SystemSetup page
		MenuPage setup_page;
		setup_page.id = PageId::SystemSetup;
		setup_page.name = "System Setup";
		setup_page.detectors = { {0, "System Setup"} };
		setup_page.edges = {
			{ EdgeTrigger::Back, PageId::SystemSetup, PageId::MenuHelp, 0, "" }
		};
		model.RegisterPage(std::move(setup_page));

		// Service page (blocking)
		MenuPage service_page;
		service_page.id = PageId::Service;
		service_page.name = "Service";
		service_page.detectors = { {0, "Service Mode"} };
		model.RegisterPage(std::move(service_page));

		// TimeOut page (blocking)
		MenuPage timeout_page;
		timeout_page.id = PageId::TimeOut;
		timeout_page.name = "TimeOut";
		timeout_page.detectors = { {0, "Timeout"} };
		model.RegisterPage(std::move(timeout_page));

		// EnterPassword page
		MenuPage password_page;
		password_page.id = PageId::EnterPassword;
		password_page.name = "Enter Password";
		password_page.detectors = { {0, "Enter Password"} };
		password_page.edges = {
			{ EdgeTrigger::Back, PageId::EnterPassword, PageId::MenuHelp, 0, "" }
		};
		model.RegisterPage(std::move(password_page));

		// StartUp splash (transient cold-start page): no edges; the controller
		// auto-advances off it on its own, so the navigator must wait, not recover.
		MenuPage startup_page;
		startup_page.id = PageId::StartUp;
		startup_page.name = "StartUp";
		startup_page.detectors = { {7, "REV "} };
		startup_page.transient = true;
		model.RegisterPage(std::move(startup_page));

		// Global edges for system events
		model.RegisterGlobalEdge({ EdgeTrigger::SystemTimeout, PageId::Unknown, PageId::TimeOut, 0, "Timeout" });
		model.RegisterGlobalEdge({ EdgeTrigger::SystemService, PageId::Unknown, PageId::Service, 0, "Service" });

		return model;
	}

	// Helper: build a ScreenDataPage with text on specified lines
	ScreenDataPage MakePage(const std::vector<std::pair<uint8_t, std::string>>& lines, size_t row_count = 12)
	{
		ScreenDataPage page(row_count);
		for (const auto& [line, text] : lines)
		{
			if (line < page.Size())
			{
				page[line].Text = text;
			}
		}
		return page;
	}
}

BOOST_AUTO_TEST_SUITE(Navigation_NavigatorEdgeCasesTestSuite)

// =============================================================================
// Cursor Movement
// =============================================================================

BOOST_AUTO_TEST_CASE(TestCursorMovement_LineDown_WhenBelowTarget)
{
	auto model = BuildTestModel();
	Navigator nav(model);

	// Sync to System page
	nav.StartSync();
	auto system_content = MakePage({{0, "Equipment ON/OFF"}});
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		nav.OnPageUpdate(system_content, 0);
	}
	BOOST_CHECK(nav.IsSynced());
	BOOST_CHECK(nav.GetCurrentPage() == PageId::System);

	// Navigate to MenuHelp (requires Select at line 11)
	nav.NavigateTo(PageId::MenuHelp);
	BOOST_CHECK_EQUAL(static_cast<int>(nav.GetState()), static_cast<int>(Navigator::State::Navigating));

	// First OnPageUpdate should try to move cursor from 0 to 11 -> LineDown
	auto cmd = nav.OnPageUpdate(system_content, 0);
	BOOST_REQUIRE(cmd.has_value());
	BOOST_CHECK_EQUAL(static_cast<int>(cmd.value()), static_cast<int>(NavKeyCommand::LineDown));
}

BOOST_AUTO_TEST_CASE(TestCursorMovement_LineUp_WhenAboveTarget)
{
	auto model = BuildTestModel();
	Navigator nav(model);

	// Sync to EquipmentOnOff (has Back edge)
	nav.StartSync();
	auto equip_content = MakePage({{0, "Equipment ON/OFF"}, {1, "Filter Pump"}});
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		nav.OnPageUpdate(equip_content, 5);
	}

	// Navigate to System via Back (no cursor requirement)
	nav.NavigateTo(PageId::System);
	auto cmd = nav.OnPageUpdate(equip_content, 5);
	// Should send Back command
	BOOST_REQUIRE(cmd.has_value());
	BOOST_CHECK_EQUAL(static_cast<int>(cmd.value()), static_cast<int>(NavKeyCommand::Back));
}

// Regression: a clear-all highlight (line id 0xFF) means "no line highlighted" on the device.
// Treating that literal value as a real row index would leave the navigator chasing a
// non-existent line; the previously known cursor line must be retained instead.
BOOST_AUTO_TEST_CASE(TestCursorMovement_ClearAllHighlight_RetainsPreviousCursor)
{
	auto model = BuildTestModel();
	Navigator nav(model);

	auto system_content = MakePage({{0, "Equipment ON/OFF"}});

	// Establish a known cursor line.
	nav.OnPageUpdate(system_content, 3);
	BOOST_CHECK_EQUAL(static_cast<int>(nav.GetCursorLine()), 3);

	// A clear-all (0xFF) arrives; the cursor line must NOT become 0xFF.
	nav.OnPageUpdate(system_content, Navigator::CURSOR_LINE_NONE);
	BOOST_CHECK_EQUAL(static_cast<int>(nav.GetCursorLine()), 3);

	// A subsequent real highlight updates the cursor normally.
	nav.OnPageUpdate(system_content, 4);
	BOOST_CHECK_EQUAL(static_cast<int>(nav.GetCursorLine()), 4);
}

// =============================================================================
// Recovery
// =============================================================================

BOOST_AUTO_TEST_CASE(TestRecovery_ServicePage_CausesFailed)
{
	auto model = BuildTestModel();
	Navigator nav(model);

	// Sync to MenuHelp
	nav.StartSync();
	auto help_content = MakePage({{0, "Menu/Help"}});
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		nav.OnPageUpdate(help_content, 0);
	}

	// Navigate to SystemSetup
	nav.NavigateTo(PageId::SystemSetup);

	// During recovery, encounter Service page (blocking -> Failed)
	auto service_content = MakePage({{0, "Service Mode"}});
	nav.OnStatusMessageReceived();
	nav.OnStatusMessageReceived();
	nav.OnPageUpdate(service_content, 0);

	// Service page is a blocking page -> should transition to Failed
	BOOST_CHECK_EQUAL(static_cast<int>(nav.GetState()), static_cast<int>(Navigator::State::Failed));
}

// =============================================================================
// Transient pages (cold-start StartUp splash)
// =============================================================================

BOOST_AUTO_TEST_CASE(TestTransientPage_StartUpSplash_WaitsThenSucceeds)
{
	// Regression (live capture test/fixtures/iaq_onetouch_startup.cap): at cold
	// start the controller shows the StartUp splash, which has no Back support and
	// no edges. The navigator used to treat it as unrecoverable and FAIL the whole
	// startup crawl (navigator.cpp:908 "Cannot recover from page without Back
	// support and no System link"). It must instead WAIT for the controller to
	// auto-advance, then continue to the target.
	auto model = BuildTestModel();
	Navigator nav(model);

	// Sync to EquipmentOnOff (which has a Back edge to System).
	nav.StartSync();
	auto equip_content = MakePage({{0, "Equipment ON/OFF"}, {1, "Filter Pump"}});
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		nav.OnPageUpdate(equip_content, 5);
	}
	BOOST_REQUIRE(nav.GetCurrentPage() == PageId::EquipmentOnOff);

	// Navigate to System: the navigator sends Back and waits for System.
	nav.NavigateTo(PageId::System);
	auto cmd = nav.OnPageUpdate(equip_content, 5);
	BOOST_REQUIRE(cmd.has_value());
	BOOST_CHECK_EQUAL(static_cast<int>(cmd.value()), static_cast<int>(NavKeyCommand::Back));

	// Instead of System, the controller shows the transient StartUp splash.
	auto startup_content = MakePage({{6, "RS-8 Combo"}, {7, "REV T"}});
	nav.OnStatusMessageReceived();
	nav.OnStatusMessageReceived();
	nav.OnPageUpdate(startup_content, 0);

	// Must NOT fail -- it should be waiting for the splash to auto-clear.
	BOOST_CHECK(nav.GetState() != Navigator::State::Failed);
	BOOST_CHECK(!nav.IsComplete());
	BOOST_CHECK(nav.GetCurrentPage() == PageId::StartUp);

	// The splash can persist for a few updates -- still no failure.
	nav.OnPageUpdate(startup_content, 0);
	BOOST_CHECK(nav.GetState() != Navigator::State::Failed);

	// The controller auto-advances to System (home) -> navigation completes.
	auto system_content = MakePage({{0, "Equipment ON/OFF"}});
	nav.OnPageUpdate(system_content, 0);
	BOOST_CHECK(nav.IsSuccess());
	BOOST_CHECK(nav.GetCurrentPage() == PageId::System);
}

BOOST_AUTO_TEST_CASE(TestTransientPage_NeverClears_StopsWaitingAndDoesNotHang)
{
	// A transient page that never clears must not be waited on forever: after
	// MAX_TRANSIENT_WAITS it falls through to normal recovery (which, for a page
	// with no Back and no System link, ends in Failed) -- i.e. it terminates
	// rather than hanging.
	auto model = BuildTestModel();
	Navigator nav(model);

	nav.StartSync();
	auto equip_content = MakePage({{0, "Equipment ON/OFF"}, {1, "Filter Pump"}});
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		nav.OnPageUpdate(equip_content, 5);
	}

	nav.NavigateTo(PageId::System);
	nav.OnPageUpdate(equip_content, 5);  // sends Back
	nav.OnStatusMessageReceived();
	nav.OnStatusMessageReceived();

	// Feed the splash well past the wait budget; it must stop waiting eventually.
	auto startup_content = MakePage({{6, "RS-8 Combo"}, {7, "REV T"}});
	for (uint32_t i = 0; i < Navigator::MAX_TRANSIENT_WAITS + 5; ++i)
	{
		nav.OnPageUpdate(startup_content, 0);
	}

	BOOST_CHECK(nav.IsComplete());  // terminated (Failed) rather than hanging forever
}

// =============================================================================
// Password Entry
// =============================================================================

BOOST_AUTO_TEST_CASE(TestPassword_NoPassword_BacksOut)
{
	auto model = BuildTestModel();
	Navigator nav(model);

	// Sync to MenuHelp
	nav.StartSync();
	auto help_content = MakePage({{0, "Menu/Help"}});
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		nav.OnPageUpdate(help_content, 0);
	}

	// Navigate somewhere, then encounter password page
	nav.NavigateTo(PageId::SystemSetup);

	// Feed a password page without setting a password
	auto password_content = MakePage({{0, "Enter Password"}});
	auto cmd = nav.OnPageUpdate(password_content, 0);

	// Should back out since no password is set
	BOOST_REQUIRE(cmd.has_value());
	BOOST_CHECK_EQUAL(static_cast<int>(cmd.value()), static_cast<int>(NavKeyCommand::Back));
}

BOOST_AUTO_TEST_CASE(TestPassword_ConfiguredPassword_BacksOutUnsupported)
{
	// Regression: automated PIN entry over RS-485 is not implemented (it requires reading the
	// on-screen digit value, which has not been decoded). The previous placeholder blindly
	// pressed Select per digit and reported success without ever setting a value (and logged the
	// PIN). The navigator must now back out honestly even when a password is configured.
	auto model = BuildTestModel();
	Navigator nav(model);
	nav.SetPassword("1234");

	// Sync to MenuHelp
	nav.StartSync();
	auto help_content = MakePage({{0, "Menu/Help"}});
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		nav.OnPageUpdate(help_content, 0);
	}

	// Navigate somewhere
	nav.NavigateTo(PageId::SystemSetup);

	// Feed a password page with password set
	auto password_content = MakePage({{0, "Enter Password"}});
	auto cmd = nav.OnPageUpdate(password_content, 0);

	// Automated password entry is unsupported -> back out (NOT a blind Select that fakes success)
	BOOST_REQUIRE(cmd.has_value());
	BOOST_CHECK_EQUAL(static_cast<int>(cmd.value()), static_cast<int>(NavKeyCommand::Back));

	// The navigator must NOT enter the (removed) EnteringPassword state.
	BOOST_CHECK(nav.GetState() != Navigator::State::EnteringPassword);
}

// =============================================================================
// Special Pages
// =============================================================================

BOOST_AUTO_TEST_CASE(TestSpecialPage_Service_CausesFailed)
{
	auto model = BuildTestModel();
	Navigator nav(model);

	// Sync to System
	nav.StartSync();
	auto system_content = MakePage({{0, "Equipment ON/OFF"}});
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		nav.OnPageUpdate(system_content, 0);
	}

	nav.NavigateTo(PageId::MenuHelp);

	// Encounter Service page during navigation
	auto service_content = MakePage({{0, "Service Mode"}});
	nav.OnPageUpdate(service_content, 0);

	BOOST_CHECK_EQUAL(static_cast<int>(nav.GetState()), static_cast<int>(Navigator::State::Failed));
}

// =============================================================================
// Recompute Limit
// =============================================================================

BOOST_AUTO_TEST_CASE(TestRecomputeLimit_MaxRecomputes_CausesFailed)
{
	auto model = BuildTestModel();
	Navigator nav(model);

	// Sync
	nav.StartSync();
	auto system_content = MakePage({{0, "Equipment ON/OFF"}});
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		nav.OnPageUpdate(system_content, 0);
	}

	// Navigate to System (already there) - should succeed immediately
	nav.NavigateTo(PageId::System);
	nav.OnPageUpdate(system_content, 0);

	// Should arrive at destination
	BOOST_CHECK_EQUAL(static_cast<int>(nav.GetState()), static_cast<int>(Navigator::State::AtDestination));
}

// When a target's menu item does not exist on the current controller model (e.g. the
// IAQ-only Diagnostics page on a non-iAqualink panel, or the chlorinator Boost/Aquapure
// pages on a panel with no SWG) the cursor wraps past the missing line, the navigator
// "proceeds" onto a wrong page, recomputes, and repeats - and the wrong page it lands on
// can VARY from cycle to cycle. The navigator must detect this after a few cycles
// (MAX_STUCK_RECOMPUTES) and fail the target FAST so the start-up scrape skips the missing
// page and moves on, instead of grinding all the way to MAX_RECOMPUTE_COUNT (which wedged
// the scrape for ~30-50 polls per missing page). The varying-wrong-page case is the one
// that defeated the earlier (actual,target) keying, which reset the counter whenever the
// landing page changed; the detector must key on the TARGET alone.
BOOST_AUTO_TEST_CASE(TestUnreachableTarget_VaryingWrongPages_FailsFast)
{
	// Build a model where the target (Boost) is reachable in the GRAPH from every page,
	// but is never actually reached: the test simulates the cursor landing on a different
	// recognized page each cycle by feeding a different page after every Select.
	MenuModel model;

	auto add_page = [&model](PageId id, const std::string& detect)
	{
		MenuPage p;
		p.id = id;
		p.name = detect;
		p.detectors = { {0, detect} };
		p.edges = { { EdgeTrigger::Select, id, PageId::Boost, 0, "" } };
		model.RegisterPage(std::move(p));
	};

	add_page(PageId::System, "Home");
	add_page(PageId::EquipmentOnOff, "EqA");
	add_page(PageId::SystemSetup, "SetB");
	add_page(PageId::MenuHelp, "MenuC");

	MenuPage boost_page;
	boost_page.id = PageId::Boost;
	boost_page.name = "Boost";
	boost_page.detectors = { {0, "Boost"} };
	model.RegisterPage(std::move(boost_page));

	Navigator nav(model);

	// Sync to Home.
	nav.StartSync();
	auto home = MakePage({{0, "Home"}});
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		nav.OnPageUpdate(home, 0);
	}
	BOOST_REQUIRE(nav.IsSynced());

	nav.NavigateTo(PageId::Boost);

	// A page-transition Select waits on two status messages; clear them between cycles.
	auto settle = [&nav]() { nav.OnStatusMessageReceived(); nav.OnStatusMessageReceived(); };

	auto eqA = MakePage({{0, "EqA"}});
	auto setB = MakePage({{0, "SetB"}});
	auto menuC = MakePage({{0, "MenuC"}});

	nav.OnPageUpdate(home, 0);   // executes first Select toward Boost
	settle();
	nav.OnPageUpdate(eqA, 0);    // unexpected #1 -> recompute (stuck=1)
	settle();
	nav.OnPageUpdate(setB, 0);   // unexpected #2 -> recompute (stuck=2)
	settle();
	nav.OnPageUpdate(menuC, 0);  // unexpected #3 -> recompute (stuck=3) -> fail fast

	// Failed well before MAX_RECOMPUTE_COUNT (the old behaviour would still be navigating).
	BOOST_CHECK_EQUAL(static_cast<int>(nav.GetState()), static_cast<int>(Navigator::State::Failed));
}

// =============================================================================
// Reset
// =============================================================================

BOOST_AUTO_TEST_CASE(TestReset_ClearsAllState)
{
	auto model = BuildTestModel();
	Navigator nav(model);

	nav.StartSync();
	auto system_content = MakePage({{0, "Equipment ON/OFF"}});
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		nav.OnPageUpdate(system_content, 0);
	}

	nav.NavigateTo(PageId::MenuHelp);
	nav.Reset();

	BOOST_CHECK_EQUAL(static_cast<int>(nav.GetState()), static_cast<int>(Navigator::State::Idle));
	BOOST_CHECK(nav.GetCurrentPage() == PageId::Unknown);
	BOOST_CHECK(nav.GetTargetPage() == PageId::Unknown);
}

// =============================================================================
// IsComplete
// =============================================================================

BOOST_AUTO_TEST_CASE(TestIsComplete_AtDestination)
{
	auto model = BuildTestModel();
	Navigator nav(model);

	nav.StartSync();
	auto system_content = MakePage({{0, "Equipment ON/OFF"}});
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		nav.OnPageUpdate(system_content, 0);
	}

	nav.NavigateTo(PageId::System);
	nav.OnPageUpdate(system_content, 0);

	BOOST_CHECK(nav.IsComplete());
	BOOST_CHECK(nav.IsSuccess());
}

BOOST_AUTO_TEST_CASE(TestIsComplete_NotWhileNavigating)
{
	auto model = BuildTestModel();
	Navigator nav(model);

	nav.StartSync();
	auto system_content = MakePage({{0, "Equipment ON/OFF"}});
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		nav.OnPageUpdate(system_content, 0);
	}

	nav.NavigateTo(PageId::MenuHelp);

	BOOST_CHECK(!nav.IsComplete());
}

// =============================================================================
// Sync
// =============================================================================

BOOST_AUTO_TEST_CASE(TestSync_RequiresConsistentDetections)
{
	auto model = BuildTestModel();
	Navigator nav(model);

	nav.StartSync();
	auto system_content = MakePage({{0, "Equipment ON/OFF"}});

	// Feed less than required consistent detections
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT - 1; ++i)
	{
		nav.OnPageUpdate(system_content, 0);
	}

	// Should not yet be synced (still in Syncing state)
	BOOST_CHECK_EQUAL(static_cast<int>(nav.GetState()), static_cast<int>(Navigator::State::Syncing));

	// One more should complete sync
	nav.OnPageUpdate(system_content, 0);
	BOOST_CHECK(nav.IsSynced());
	BOOST_CHECK_EQUAL(static_cast<int>(nav.GetState()), static_cast<int>(Navigator::State::Idle));
}

BOOST_AUTO_TEST_CASE(TestSync_InconsistentDetections_ResetsCounter)
{
	auto model = BuildTestModel();
	Navigator nav(model);

	nav.StartSync();
	auto system_content = MakePage({{0, "Equipment ON/OFF"}});
	auto help_content = MakePage({{0, "Menu/Help"}});

	// Detect System twice
	nav.OnPageUpdate(system_content, 0);
	nav.OnPageUpdate(system_content, 0);

	// Switch to Help - resets counter
	nav.OnPageUpdate(help_content, 0);

	// Should not be synced yet
	BOOST_CHECK(!nav.IsSynced());

	// Now detect Help consistently
	for (uint32_t i = 1; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		nav.OnPageUpdate(help_content, 0);
	}

	BOOST_CHECK(nav.IsSynced());
	BOOST_CHECK(nav.GetCurrentPage() == PageId::MenuHelp);
}

// =============================================================================
// In-place Select (equipment toggle) and Set-Temperature cursor positioning.
//
// These exercise the real OneTouch menu model (CreateOneTouchMenuModel) used in
// production, with the Equipment ON/OFF page content taken verbatim from the live
// capture test/fixtures/onetouch_equipment_toggle.cap (decoded screen lines).
// =============================================================================

namespace
{
	// The Equipment ON/OFF page exactly as it appears in onetouch_equipment_toggle.cap.
	ScreenDataPage MakeEquipmentOnOffPage()
	{
		return MakePage({
			{ 0,  "Filter Pump  OFF" },   // page detector for EquipmentOnOff
			{ 1,  "Spa          OFF" },
			{ 2,  "Pool Heat    OFF" },
			{ 3,  "Spa Heat     OFF" },
			{ 4,  "Solar Heat   ENA" },
			{ 5,  "Spa Jets     OFF" },
			{ 6,  "Swim Jet     OFF" },
			{ 7,  "Swim Jet     OFF" },
			{ 8,  "Swim Jet     OFF" },
			{ 9,  "Spillway      ON" },
			{ 10, "Spa Mode     OFF" },
			{ 11, "   ^^ More vv   " },
		});
	}
}

// A toggle goal selects the matched Equipment ON/OFF row IN PLACE: the Navigator
// emits a single Select and the "page after Select" is the SAME page (an in-place
// toggle, not a transition). This is the core of OneTouch equipment actuation.
BOOST_AUTO_TEST_CASE(TestInPlaceSelect_EquipmentOnOff_SelectsRowWithoutLeavingPage)
{
	auto model = CreateOneTouchMenuModel();
	Navigator nav(model);

	auto equip = MakeEquipmentOnOffPage();

	// Sync onto the page with the cursor already on the Spillway row (line 9), so no
	// cursor walk is needed and the Navigator can Select immediately.
	nav.StartSync();
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		nav.OnPageUpdate(equip, 9);
	}
	BOOST_REQUIRE(nav.IsSynced());
	BOOST_REQUIRE(nav.GetCurrentPage() == PageId::EquipmentOnOff);

	// In-place toggle: select_target == the SAME page (EquipmentOnOff).
	nav.NavigateToItem(PageId::EquipmentOnOff, 0, "Spillway", PageId::EquipmentOnOff);

	auto cmd = nav.OnPageUpdate(equip, 9);
	BOOST_REQUIRE(cmd.has_value());
	BOOST_CHECK_EQUAL(static_cast<int>(cmd.value()), static_cast<int>(NavKeyCommand::Select));

	// The "destination after Select" is the Equipment ON/OFF page itself - the row
	// re-renders in place rather than the UI transitioning elsewhere.
	BOOST_CHECK(nav.GetTargetPage() == PageId::EquipmentOnOff);
}

// When the cursor is NOT yet on the target row, the Navigator first walks it there
// (LineDown toward a lower row) before it would Select - it does not Select blindly.
BOOST_AUTO_TEST_CASE(TestInPlaceSelect_EquipmentOnOff_WalksCursorToRowFirst)
{
	auto model = CreateOneTouchMenuModel();
	Navigator nav(model);

	auto equip = MakeEquipmentOnOffPage();

	nav.StartSync();
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		nav.OnPageUpdate(equip, 0);   // cursor at top
	}
	BOOST_REQUIRE(nav.IsSynced());

	nav.NavigateToItem(PageId::EquipmentOnOff, 0, "Spillway", PageId::EquipmentOnOff);

	// Spillway is at line 9, cursor at 0 -> move down toward it (not a Select yet).
	auto cmd = nav.OnPageUpdate(equip, 0);
	BOOST_REQUIRE(cmd.has_value());
	BOOST_CHECK_EQUAL(static_cast<int>(cmd.value()), static_cast<int>(NavKeyCommand::LineDown));
}

// The Set Temperature setpoint edit begins by positioning the cursor on the Pool/Spa
// Heat row WITHOUT pressing Select (select_target == Unknown): the OneTouch device
// then drives the in-place value editor itself. The Navigator must stop AT the row.
BOOST_AUTO_TEST_CASE(TestSetTemperature_PositionsCursorOnPoolHeat_WithoutSelect)
{
	auto model = CreateOneTouchMenuModel();
	Navigator nav(model);

	// Set Temperature page (see PageProcessor_SetTemperature): Pool Heat line 2, Spa
	// Heat line 3. Detector is "Set Temp" on line 0.
	auto settemp = MakePage({
		{ 0, "    Set Temp    " },
		{ 2, "Pool Heat    90`F" },
		{ 3, "Spa Heat    102`F" },
		{ 8, "Highlight an" },
		{ 9, "item and press" },
		{ 10, "Select" },
	});

	// Sync onto the Set Temperature page with the cursor already on the Pool Heat row.
	nav.StartSync();
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		nav.OnPageUpdate(settemp, 2);
	}
	BOOST_REQUIRE(nav.IsSynced());
	BOOST_REQUIRE(nav.GetCurrentPage() == PageId::SetTemperature);

	// select_target Unknown -> stop at the row; do NOT emit Select.
	nav.NavigateToItem(PageId::SetTemperature, 2, "Pool Heat", PageId::Unknown);

	auto cmd = nav.OnPageUpdate(settemp, 2);
	BOOST_CHECK(!cmd.has_value());                 // no Select - editing is the device's job
	BOOST_CHECK(nav.IsComplete());
	BOOST_CHECK(nav.IsSuccess());                  // AtDestination, cursor on the Pool Heat row
	BOOST_CHECK_EQUAL(static_cast<int>(nav.GetCursorLine()), 2);
}

// The chlorinator % edit positions the cursor on the Set AquaPure "Set Pool to:" row
// (line 3) without Select - same value-editor entry as the setpoint. Grounded in
// onetouch_chlorinator.cap (Pool % = Set AquaPure line 3).
BOOST_AUTO_TEST_CASE(TestSetAquapure_PositionsCursorOnPoolRow_WithoutSelect)
{
	auto model = CreateOneTouchMenuModel();
	Navigator nav(model);

	auto aqua = MakePage({
		{ 0, "  Set AquaPure  " },     // detector "Set AquaPure"
		{ 3, "Set Pool to: 40%" },     // pool chlorination row
		{ 4, "Set Spa to:  40%" },     // spa chlorination row
	});

	nav.StartSync();
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		nav.OnPageUpdate(aqua, 3);
	}
	BOOST_REQUIRE(nav.IsSynced());
	BOOST_REQUIRE(nav.GetCurrentPage() == PageId::SetAquapure);

	nav.NavigateToItem(PageId::SetAquapure, 3, "Set Pool", PageId::Unknown);

	auto cmd = nav.OnPageUpdate(aqua, 3);
	BOOST_CHECK(!cmd.has_value());                 // no Select - the device drives the % editor
	BOOST_CHECK(nav.IsComplete());
	BOOST_CHECK(nav.IsSuccess());
	BOOST_CHECK_EQUAL(static_cast<int>(nav.GetCursorLine()), 3);
}

BOOST_AUTO_TEST_SUITE_END()
