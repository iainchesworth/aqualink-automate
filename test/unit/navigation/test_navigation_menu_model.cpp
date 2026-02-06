#include <boost/test/unit_test.hpp>

#include "navigation/menu_model.h"
#include "navigation/onetouch_menu_model.h"

using namespace AqualinkAutomate::Navigation;
using namespace AqualinkAutomate::Utility;

BOOST_AUTO_TEST_SUITE(MenuModel_TestSuite)

// =============================================================================
// Menu Model Line Number Tests
// These tests verify that menu items have correct line numbers to prevent
// navigation issues when cursor positions don't match expected targets.
// =============================================================================

BOOST_AUTO_TEST_CASE(TestSystemPageItemLineNumbers)
{
	auto model = CreateOneTouchMenuModel();
	const MenuPage* system_page = model.GetPage(PageId::System);

	BOOST_REQUIRE(system_page != nullptr);
	BOOST_REQUIRE_EQUAL(system_page->items.size(), 3);

	// System page items at lines 9-11 (after status display area)
	BOOST_CHECK_EQUAL(system_page->items[0].label, "Equipment ON/OFF");
	BOOST_CHECK_EQUAL(system_page->items[0].line, 9);
	BOOST_CHECK(system_page->items[0].target == PageId::EquipmentOnOff);

	BOOST_CHECK_EQUAL(system_page->items[1].label, "OneTouch ON/OFF");
	BOOST_CHECK_EQUAL(system_page->items[1].line, 10);
	BOOST_CHECK(system_page->items[1].target == PageId::OneTouch);

	BOOST_CHECK_EQUAL(system_page->items[2].label, "Menu/Help");
	BOOST_CHECK_EQUAL(system_page->items[2].line, 11);
	BOOST_CHECK(system_page->items[2].target == PageId::MenuHelp);
}

BOOST_AUTO_TEST_CASE(TestMenuHelpPageItemLineNumbers)
{
	auto model = CreateOneTouchMenuModel();
	const MenuPage* menu_page = model.GetPage(PageId::MenuHelp);

	BOOST_REQUIRE(menu_page != nullptr);
	BOOST_REQUIRE_EQUAL(menu_page->items.size(), 10);

	// Menu/Help page items starting at line 1 (line 0 is "Menu" title)
	BOOST_CHECK_EQUAL(menu_page->items[0].label, "Help");
	BOOST_CHECK_EQUAL(menu_page->items[0].line, 1);

	BOOST_CHECK_EQUAL(menu_page->items[1].label, "Program");
	BOOST_CHECK_EQUAL(menu_page->items[1].line, 2);

	BOOST_CHECK_EQUAL(menu_page->items[2].label, "Set Temp");
	BOOST_CHECK_EQUAL(menu_page->items[2].line, 3);

	BOOST_CHECK_EQUAL(menu_page->items[3].label, "Set Time");
	BOOST_CHECK_EQUAL(menu_page->items[3].line, 4);

	BOOST_CHECK_EQUAL(menu_page->items[4].label, "Set AquaPure");
	BOOST_CHECK_EQUAL(menu_page->items[4].line, 5);

	BOOST_CHECK_EQUAL(menu_page->items[5].label, "Display Light");
	BOOST_CHECK_EQUAL(menu_page->items[5].line, 6);

	BOOST_CHECK_EQUAL(menu_page->items[6].label, "Lockouts");
	BOOST_CHECK_EQUAL(menu_page->items[6].line, 7);

	BOOST_CHECK_EQUAL(menu_page->items[7].label, "Password");
	BOOST_CHECK_EQUAL(menu_page->items[7].line, 8);

	BOOST_CHECK_EQUAL(menu_page->items[8].label, "Program Group");
	BOOST_CHECK_EQUAL(menu_page->items[8].line, 9);

	BOOST_CHECK_EQUAL(menu_page->items[9].label, "System Setup");
	BOOST_CHECK_EQUAL(menu_page->items[9].line, 10);
	BOOST_CHECK(menu_page->items[9].target == PageId::SystemSetup);
}

BOOST_AUTO_TEST_CASE(TestSystemSetupPageItemLineNumbers)
{
	// CRITICAL: This test was added after fixing a bug where "Label Aux"
	// was incorrectly at line 1, causing navigation to select wrong items.
	// The first selectable item is at line 2 (line 0 = title, line 1 = blank)
	auto model = CreateOneTouchMenuModel();
	const MenuPage* setup_page = model.GetPage(PageId::SystemSetup);

	BOOST_REQUIRE(setup_page != nullptr);
	BOOST_REQUIRE_GE(setup_page->items.size(), 1);

	// Label Aux should be at line 2, NOT line 1
	BOOST_CHECK_EQUAL(setup_page->items[0].label, "Label Aux");
	BOOST_CHECK_EQUAL(setup_page->items[0].line, 2);
	BOOST_CHECK(setup_page->items[0].target == PageId::LabelAuxList);
}

BOOST_AUTO_TEST_CASE(TestLabelAuxListPageItemLineNumbers)
{
	// CRITICAL: This test was added after fixing a bug where AUX items
	// started at line 1, but the first selectable item is at line 2.
	auto model = CreateOneTouchMenuModel();
	const MenuPage* aux_list_page = model.GetPage(PageId::LabelAuxList);

	BOOST_REQUIRE(aux_list_page != nullptr);
	BOOST_REQUIRE_GE(aux_list_page->items.size(), 7);

	// AUX items should start at line 2 (line 0 = title, line 1 = blank)
	BOOST_CHECK_EQUAL(aux_list_page->items[0].label, "AUX 1");
	BOOST_CHECK_EQUAL(aux_list_page->items[0].line, 2);

	BOOST_CHECK_EQUAL(aux_list_page->items[1].label, "AUX 2");
	BOOST_CHECK_EQUAL(aux_list_page->items[1].line, 3);

	BOOST_CHECK_EQUAL(aux_list_page->items[2].label, "AUX 3");
	BOOST_CHECK_EQUAL(aux_list_page->items[2].line, 4);

	BOOST_CHECK_EQUAL(aux_list_page->items[3].label, "AUX 4");
	BOOST_CHECK_EQUAL(aux_list_page->items[3].line, 5);

	BOOST_CHECK_EQUAL(aux_list_page->items[4].label, "AUX 5");
	BOOST_CHECK_EQUAL(aux_list_page->items[4].line, 6);

	BOOST_CHECK_EQUAL(aux_list_page->items[5].label, "AUX 6");
	BOOST_CHECK_EQUAL(aux_list_page->items[5].line, 7);

	BOOST_CHECK_EQUAL(aux_list_page->items[6].label, "AUX 7");
	BOOST_CHECK_EQUAL(aux_list_page->items[6].line, 8);

	// All AUX items should target LabelAux page
	for (size_t i = 0; i < aux_list_page->items.size(); ++i)
	{
		BOOST_CHECK(aux_list_page->items[i].target == PageId::LabelAux);
	}
}

BOOST_AUTO_TEST_CASE(TestHelpSubmenuPageItemLineNumbers)
{
	auto model = CreateOneTouchMenuModel();
	const MenuPage* help_page = model.GetPage(PageId::HelpSubmenu);

	BOOST_REQUIRE(help_page != nullptr);
	BOOST_REQUIRE_EQUAL(help_page->items.size(), 3);

	// Help submenu items at lines 1-3 (line 0 is "Help" title)
	BOOST_CHECK_EQUAL(help_page->items[0].label, "Keys");
	BOOST_CHECK_EQUAL(help_page->items[0].line, 1);

	BOOST_CHECK_EQUAL(help_page->items[1].label, "Service");
	BOOST_CHECK_EQUAL(help_page->items[1].line, 2);

	BOOST_CHECK_EQUAL(help_page->items[2].label, "Diagnostics");
	BOOST_CHECK_EQUAL(help_page->items[2].line, 3);
}

// =============================================================================
// Page Detection Tests
// =============================================================================

BOOST_AUTO_TEST_CASE(TestPageDetectionPatterns)
{
	auto model = CreateOneTouchMenuModel();

	// Verify key pages have detection patterns
	const MenuPage* system_page = model.GetPage(PageId::System);
	BOOST_REQUIRE(system_page != nullptr);
	BOOST_REQUIRE_GE(system_page->detectors.size(), 1);
	BOOST_CHECK_EQUAL(system_page->detectors[0].line, 9);
	BOOST_CHECK_EQUAL(system_page->detectors[0].pattern, "Equipment ON/OFF");

	const MenuPage* menu_page = model.GetPage(PageId::MenuHelp);
	BOOST_REQUIRE(menu_page != nullptr);
	BOOST_REQUIRE_GE(menu_page->detectors.size(), 1);
	BOOST_CHECK_EQUAL(menu_page->detectors[0].line, 0);
	BOOST_CHECK_EQUAL(menu_page->detectors[0].pattern, "Menu");

	const MenuPage* system_setup = model.GetPage(PageId::SystemSetup);
	BOOST_REQUIRE(system_setup != nullptr);
	BOOST_REQUIRE_GE(system_setup->detectors.size(), 1);
	BOOST_CHECK_EQUAL(system_setup->detectors[0].line, 0);
	BOOST_CHECK_EQUAL(system_setup->detectors[0].pattern, "System Setup");

	const MenuPage* label_aux_list = model.GetPage(PageId::LabelAuxList);
	BOOST_REQUIRE(label_aux_list != nullptr);
	BOOST_REQUIRE_GE(label_aux_list->detectors.size(), 1);
	BOOST_CHECK_EQUAL(label_aux_list->detectors[0].line, 0);
	BOOST_CHECK_EQUAL(label_aux_list->detectors[0].pattern, "Label Aux");
}

BOOST_AUTO_TEST_CASE(TestEnterPasswordPageDetection)
{
	// Verify Enter Password page is properly registered for detection
	auto model = CreateOneTouchMenuModel();
	const MenuPage* password_page = model.GetPage(PageId::EnterPassword);

	BOOST_REQUIRE(password_page != nullptr);
	BOOST_REQUIRE_GE(password_page->detectors.size(), 1);
	BOOST_CHECK_EQUAL(password_page->detectors[0].line, 0);
	BOOST_CHECK_EQUAL(password_page->detectors[0].pattern, "Enter Password");

	// Parent should be MenuHelp so Back works
	BOOST_CHECK(password_page->parent.has_value());
	BOOST_CHECK(password_page->parent.value() == PageId::MenuHelp);
}

// =============================================================================
// Page Hierarchy Tests
// =============================================================================

BOOST_AUTO_TEST_CASE(TestPageParentRelationships)
{
	auto model = CreateOneTouchMenuModel();

	// System is root - no parent
	const MenuPage* system_page = model.GetPage(PageId::System);
	BOOST_REQUIRE(system_page != nullptr);
	BOOST_CHECK(!system_page->parent.has_value());

	// MenuHelp parent is System
	const MenuPage* menu_page = model.GetPage(PageId::MenuHelp);
	BOOST_REQUIRE(menu_page != nullptr);
	BOOST_CHECK(menu_page->parent.has_value());
	BOOST_CHECK(menu_page->parent.value() == PageId::System);

	// SystemSetup parent is MenuHelp
	const MenuPage* setup_page = model.GetPage(PageId::SystemSetup);
	BOOST_REQUIRE(setup_page != nullptr);
	BOOST_CHECK(setup_page->parent.has_value());
	BOOST_CHECK(setup_page->parent.value() == PageId::MenuHelp);

	// LabelAuxList parent is SystemSetup
	const MenuPage* aux_list = model.GetPage(PageId::LabelAuxList);
	BOOST_REQUIRE(aux_list != nullptr);
	BOOST_CHECK(aux_list->parent.has_value());
	BOOST_CHECK(aux_list->parent.value() == PageId::SystemSetup);

	// LabelAux parent is LabelAuxList
	const MenuPage* label_aux = model.GetPage(PageId::LabelAux);
	BOOST_REQUIRE(label_aux != nullptr);
	BOOST_CHECK(label_aux->parent.has_value());
	BOOST_CHECK(label_aux->parent.value() == PageId::LabelAuxList);
}

BOOST_AUTO_TEST_CASE(TestOneTouchStylePagesHaveNoParent)
{
	// OneTouch-style pages don't support Back button
	auto model = CreateOneTouchMenuModel();

	const MenuPage* onetouch_page = model.GetPage(PageId::OneTouch);
	BOOST_REQUIRE(onetouch_page != nullptr);
	BOOST_CHECK(!onetouch_page->parent.has_value());

	const MenuPage* equipment_page = model.GetPage(PageId::EquipmentOnOff);
	BOOST_REQUIRE(equipment_page != nullptr);
	BOOST_CHECK(!equipment_page->parent.has_value());
}

BOOST_AUTO_TEST_CASE(TestOneTouchStylePagesUseRestrictedNavSteps)
{
	auto model = CreateOneTouchMenuModel();
	auto restricted_steps = OneTouchNavStepTypes();

	const MenuPage* onetouch_page = model.GetPage(PageId::OneTouch);
	BOOST_REQUIRE(onetouch_page != nullptr);
	BOOST_CHECK(onetouch_page->allowed_steps == restricted_steps);
	BOOST_CHECK(onetouch_page->allowed_steps.count(NavStepType::Back) == 0);

	const MenuPage* equipment_page = model.GetPage(PageId::EquipmentOnOff);
	BOOST_REQUIRE(equipment_page != nullptr);
	BOOST_CHECK(equipment_page->allowed_steps == restricted_steps);
	BOOST_CHECK(equipment_page->allowed_steps.count(NavStepType::Back) == 0);
}

// =============================================================================
// Pathfinding Tests
// =============================================================================

BOOST_AUTO_TEST_CASE(TestPathfindingSystemToLabelAuxList)
{
	auto model = CreateOneTouchMenuModel();

	// Path: System -> MenuHelp -> SystemSetup -> LabelAuxList
	auto path = model.FindPath(PageId::System, PageId::LabelAuxList);

	BOOST_REQUIRE(!path.empty());

	// First step: System -> MenuHelp (Select at line 11)
	bool found_menu_step = false;
	for (const auto& step : path)
	{
		if (step.from_page == PageId::System && step.to_page == PageId::MenuHelp)
		{
			found_menu_step = true;
			BOOST_CHECK(step.type == NavStepType::Select);
			BOOST_CHECK_EQUAL(step.target_line, 11);
		}
	}
	BOOST_CHECK(found_menu_step);
}

BOOST_AUTO_TEST_CASE(TestPathfindingWithCorrectLineNumbers)
{
	auto model = CreateOneTouchMenuModel();

	// This test verifies that pathfinding uses the correct line numbers
	// Path: System -> MenuHelp -> SystemSetup
	auto path = model.FindPath(PageId::System, PageId::SystemSetup);

	BOOST_REQUIRE(!path.empty());

	// Find the MenuHelp -> SystemSetup step
	for (const auto& step : path)
	{
		if (step.from_page == PageId::MenuHelp && step.to_page == PageId::SystemSetup)
		{
			BOOST_CHECK(step.type == NavStepType::Select);
			// System Setup is at line 10 in the menu
			BOOST_CHECK_EQUAL(step.target_line, 10);
		}
	}
}

BOOST_AUTO_TEST_CASE(TestPathfindingFromSamePageIsEmpty)
{
	auto model = CreateOneTouchMenuModel();

	// No path needed when already at destination
	auto path = model.FindPath(PageId::System, PageId::System);
	BOOST_CHECK(path.empty());
}

BOOST_AUTO_TEST_CASE(TestPathfindingBetweenValidPagesSucceeds)
{
	auto model = CreateOneTouchMenuModel();

	// There should be a path from System to LabelAux
	auto path = model.FindPath(PageId::System, PageId::LabelAux);
	BOOST_CHECK(!path.empty());
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// Navigator Integration Tests
// =============================================================================

BOOST_AUTO_TEST_SUITE(Navigator_TestSuite)

BOOST_AUTO_TEST_CASE(TestMenuModelAllPagesRegistered)
{
	auto model = CreateOneTouchMenuModel();
	const auto& pages = model.GetAllPages();

	// Verify minimum expected pages are registered
	BOOST_CHECK_GE(pages.size(), 15);

	// Verify critical pages exist
	BOOST_CHECK(model.GetPage(PageId::System) != nullptr);
	BOOST_CHECK(model.GetPage(PageId::OneTouch) != nullptr);
	BOOST_CHECK(model.GetPage(PageId::MenuHelp) != nullptr);
	BOOST_CHECK(model.GetPage(PageId::SystemSetup) != nullptr);
	BOOST_CHECK(model.GetPage(PageId::LabelAuxList) != nullptr);
	BOOST_CHECK(model.GetPage(PageId::LabelAux) != nullptr);
	BOOST_CHECK(model.GetPage(PageId::EquipmentOnOff) != nullptr);
	BOOST_CHECK(model.GetPage(PageId::EnterPassword) != nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
