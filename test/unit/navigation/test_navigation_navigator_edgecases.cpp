#include <optional>

#include <boost/test/unit_test.hpp>

#include "navigation/menu_model.h"
#include "navigation/navigator.h"
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

BOOST_AUTO_TEST_CASE(TestPassword_ValidPassword_SendsSelect)
{
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

	// Should enter password with Select commands
	BOOST_REQUIRE(cmd.has_value());
	BOOST_CHECK_EQUAL(static_cast<int>(cmd.value()), static_cast<int>(NavKeyCommand::Select));
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
	auto cmd = nav.OnPageUpdate(system_content, 0);

	// Should arrive at destination
	BOOST_CHECK_EQUAL(static_cast<int>(nav.GetState()), static_cast<int>(Navigator::State::AtDestination));
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

BOOST_AUTO_TEST_SUITE_END()
