#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <set>
#include <unordered_map>
#include <vector>

#include "navigation/menu_model.h"
#include "navigation/navigator.h"
#include "navigation/onetouch_menu_model.h"
#include "navigation/spider_engine.h"
#include "navigation/visit_policies.h"

using namespace AqualinkAutomate::Navigation;
using namespace AqualinkAutomate::Utility;

// =============================================================================
// Helper: simulate page content that matches a specific page's detectors
// =============================================================================
static ScreenDataPage MakePageContent(const MenuModel& model, PageId page_id)
{
	ScreenDataPage page(12);
	const MenuPage* menu_page = model.GetPage(page_id);
	if (menu_page)
	{
		for (const auto& detector : menu_page->detectors)
		{
			if (detector.line < page.Size())
			{
				page[detector.line].Text = detector.pattern;
			}
		}
	}
	return page;
}

// =============================================================================
// Helper: feed N consistent page updates to sync the Navigator
// =============================================================================
static void SyncNavigatorToPage(Navigator& nav, const MenuModel& model, PageId page_id)
{
	auto page = MakePageContent(model, page_id);
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT + 1; ++i)
	{
		nav.OnPageUpdate(page, 0);
	}
}

BOOST_AUTO_TEST_SUITE(SpiderEngine_TestSuite)

// =============================================================================
// VisitPolicy Tests
// =============================================================================

BOOST_AUTO_TEST_CASE(TestFullDiscoveryPolicyShouldVisitNormalPages)
{
	auto model = CreateOneTouchMenuModel();
	FullDiscoveryVisitPolicy policy;

	// Normal pages should be visited
	const MenuPage* system = model.GetPage(PageId::System);
	BOOST_REQUIRE(system != nullptr);
	BOOST_CHECK(policy.ShouldVisit(PageId::System, *system));

	const MenuPage* menu_help = model.GetPage(PageId::MenuHelp);
	BOOST_REQUIRE(menu_help != nullptr);
	BOOST_CHECK(policy.ShouldVisit(PageId::MenuHelp, *menu_help));

	const MenuPage* label_aux = model.GetPage(PageId::LabelAuxList);
	BOOST_REQUIRE(label_aux != nullptr);
	BOOST_CHECK(policy.ShouldVisit(PageId::LabelAuxList, *label_aux));
}

BOOST_AUTO_TEST_CASE(TestFullDiscoveryPolicySkipsSpecialPages)
{
	auto model = CreateOneTouchMenuModel();
	FullDiscoveryVisitPolicy policy;

	// Special pages should not be visited
	const MenuPage* startup = model.GetPage(PageId::StartUp);
	BOOST_REQUIRE(startup != nullptr);
	BOOST_CHECK(!policy.ShouldVisit(PageId::StartUp, *startup));

	const MenuPage* service = model.GetPage(PageId::Service);
	BOOST_REQUIRE(service != nullptr);
	BOOST_CHECK(!policy.ShouldVisit(PageId::Service, *service));

	const MenuPage* timeout = model.GetPage(PageId::TimeOut);
	BOOST_REQUIRE(timeout != nullptr);
	BOOST_CHECK(!policy.ShouldVisit(PageId::TimeOut, *timeout));

	const MenuPage* password = model.GetPage(PageId::EnterPassword);
	BOOST_REQUIRE(password != nullptr);
	BOOST_CHECK(!policy.ShouldVisit(PageId::EnterPassword, *password));
}

BOOST_AUTO_TEST_CASE(TestTargetedPolicyVisitsOnlySpecifiedPages)
{
	auto model = CreateOneTouchMenuModel();

	std::set<PageId> targets = { PageId::System, PageId::MenuHelp, PageId::EquipmentOnOff };
	TargetedVisitPolicy policy(targets);

	const MenuPage* system = model.GetPage(PageId::System);
	BOOST_REQUIRE(system != nullptr);
	BOOST_CHECK(policy.ShouldVisit(PageId::System, *system));

	const MenuPage* menu_help = model.GetPage(PageId::MenuHelp);
	BOOST_REQUIRE(menu_help != nullptr);
	BOOST_CHECK(policy.ShouldVisit(PageId::MenuHelp, *menu_help));

	const MenuPage* equip = model.GetPage(PageId::EquipmentOnOff);
	BOOST_REQUIRE(equip != nullptr);
	BOOST_CHECK(policy.ShouldVisit(PageId::EquipmentOnOff, *equip));

	// Not in target set
	const MenuPage* label_aux = model.GetPage(PageId::LabelAuxList);
	BOOST_REQUIRE(label_aux != nullptr);
	BOOST_CHECK(!policy.ShouldVisit(PageId::LabelAuxList, *label_aux));
}

BOOST_AUTO_TEST_CASE(TestVisitPolicyCallbacks)
{
	std::vector<PageId> visited_pages;
	bool crawl_complete = false;

	FullDiscoveryVisitPolicy policy(
		[&](PageId page, const ScreenDataPage& content)
		{
			visited_pages.push_back(page);
		},
		[&]()
		{
			crawl_complete = true;
		}
	);

	ScreenDataPage content(12);
	policy.OnPageReached(PageId::System, content);
	policy.OnPageReached(PageId::MenuHelp, content);

	BOOST_REQUIRE_EQUAL(visited_pages.size(), 2);
	BOOST_CHECK(visited_pages[0] == PageId::System);
	BOOST_CHECK(visited_pages[1] == PageId::MenuHelp);
	BOOST_CHECK(!crawl_complete);

	policy.OnCrawlComplete();
	BOOST_CHECK(crawl_complete);
}

// =============================================================================
// SpiderEngine State Tests
// =============================================================================

BOOST_AUTO_TEST_CASE(TestSpiderEngineStartsIdle)
{
	auto model = CreateOneTouchMenuModel();
	Navigator nav(model);
	SpiderEngine engine(model, nav);

	BOOST_CHECK(engine.GetState() == SpiderEngine::State::Idle);
	BOOST_CHECK(engine.GetVisitedPages().empty());
}

BOOST_AUTO_TEST_CASE(TestSpiderEngineStartCrawlWithUnsyncedNavigator)
{
	auto model = CreateOneTouchMenuModel();
	Navigator nav(model);
	SpiderEngine engine(model, nav);

	// Navigator is not synced - spider should start syncing
	auto policy = std::make_unique<FullDiscoveryVisitPolicy>();
	engine.StartCrawl(std::move(policy));

	BOOST_CHECK(engine.GetState() == SpiderEngine::State::Syncing);
}

BOOST_AUTO_TEST_CASE(TestSpiderEngineStartCrawlWithSyncedNavigator)
{
	auto model = CreateOneTouchMenuModel();
	Navigator nav(model);

	// Sync Navigator first
	nav.StartSync();
	SyncNavigatorToPage(nav, model, PageId::System);
	BOOST_REQUIRE(nav.IsSynced());

	SpiderEngine engine(model, nav);
	auto policy = std::make_unique<TargetedVisitPolicy>(
		std::set<PageId>{ PageId::System }
	);
	engine.StartCrawl(std::move(policy));

	// Already on System page and it's the only target - should capture immediately
	// After NavigateToNextTarget sees we're already there, state goes to CapturingPage
	BOOST_CHECK(engine.GetState() == SpiderEngine::State::CapturingPage ||
		engine.GetState() == SpiderEngine::State::NavigatingToNext);
}

BOOST_AUTO_TEST_CASE(TestSpiderEngineTargetedCrawlSinglePage)
{
	auto model = CreateOneTouchMenuModel();
	Navigator nav(model);

	// Sync to System page
	nav.StartSync();
	SyncNavigatorToPage(nav, model, PageId::System);
	BOOST_REQUIRE(nav.IsSynced());
	BOOST_CHECK(nav.GetCurrentPage() == PageId::System);

	// Create spider targeting only System page (already there)
	std::vector<PageId> captured;
	bool complete = false;

	SpiderEngine engine(model, nav);
	auto policy = std::make_unique<TargetedVisitPolicy>(
		std::set<PageId>{ PageId::System },
		[&](PageId page, const ScreenDataPage& content) { captured.push_back(page); },
		[&]() { complete = true; }
	);
	engine.StartCrawl(std::move(policy));

	// Process one step - should capture System page and complete
	auto system_content = MakePageContent(model, PageId::System);
	engine.ProcessStep(system_content, 0);

	// Should have captured System and completed
	BOOST_CHECK_EQUAL(captured.size(), 1);
	BOOST_CHECK(captured[0] == PageId::System);
	BOOST_CHECK(complete);
	BOOST_CHECK(engine.GetState() == SpiderEngine::State::Complete);
}

BOOST_AUTO_TEST_CASE(TestSpiderEngineChoosesNearestTarget)
{
	auto model = CreateOneTouchMenuModel();

	// Verify that the nearest target is chosen
	// From System, MenuHelp (1 step) is closer than LabelAuxList (3 steps)
	auto path_to_menu = model.FindPath(PageId::System, PageId::MenuHelp);
	auto path_to_aux = model.FindPath(PageId::System, PageId::LabelAuxList);

	BOOST_REQUIRE(!path_to_menu.empty());
	BOOST_REQUIRE(!path_to_aux.empty());
	BOOST_CHECK(path_to_menu.size() < path_to_aux.size());
}

BOOST_AUTO_TEST_CASE(TestSpiderEngineSyncThenCrawl)
{
	auto model = CreateOneTouchMenuModel();
	Navigator nav(model);
	SpiderEngine engine(model, nav);

	std::vector<PageId> captured;
	auto policy = std::make_unique<TargetedVisitPolicy>(
		std::set<PageId>{ PageId::System },
		[&](PageId page, const ScreenDataPage& content) { captured.push_back(page); },
		nullptr
	);
	engine.StartCrawl(std::move(policy));

	BOOST_CHECK(engine.GetState() == SpiderEngine::State::Syncing);

	// Feed consistent System page detections to sync
	auto system_content = MakePageContent(model, PageId::System);
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT; ++i)
	{
		engine.ProcessStep(system_content, 0);
	}

	// After sync, should capture System (already there) and complete
	// May need one more ProcessStep to trigger capture
	if (engine.GetState() != SpiderEngine::State::Complete)
	{
		engine.ProcessStep(system_content, 0);
	}

	BOOST_CHECK(engine.GetState() == SpiderEngine::State::Complete);
	BOOST_CHECK_GE(captured.size(), 1);
	BOOST_CHECK(captured[0] == PageId::System);
}

// =============================================================================
// Crawl Target Filtering Tests
// =============================================================================

BOOST_AUTO_TEST_CASE(TestFullDiscoveryPolicyAllowsUnreachablePages)
{
	// FullDiscoveryVisitPolicy doesn't know about graph reachability.
	// It allows all non-special pages, including those with no incoming edges.
	auto model = CreateOneTouchMenuModel();
	FullDiscoveryVisitPolicy policy;

	// These pages have no incoming edges but are still "normal" pages
	const MenuPage* set_aquapure = model.GetPage(PageId::SetAquapure);
	BOOST_REQUIRE(set_aquapure != nullptr);
	BOOST_CHECK(policy.ShouldVisit(PageId::SetAquapure, *set_aquapure));

	const MenuPage* more_onetouch = model.GetPage(PageId::MoreOneTouch);
	BOOST_REQUIRE(more_onetouch != nullptr);
	BOOST_CHECK(policy.ShouldVisit(PageId::MoreOneTouch, *more_onetouch));

	const MenuPage* custom_label = model.GetPage(PageId::CustomLabel);
	BOOST_REQUIRE(custom_label != nullptr);
	BOOST_CHECK(policy.ShouldVisit(PageId::CustomLabel, *custom_label));
}

BOOST_AUTO_TEST_CASE(TestUnreachablePagesHaveNoPathFromSystem)
{
	// Even though the policy allows these pages, SpiderEngine can't reach them
	// because no Select edge in the model targets them.
	auto model = CreateOneTouchMenuModel();

	auto path_aquapure = model.FindPath(PageId::System, PageId::SetAquapure);
	BOOST_CHECK(path_aquapure.empty());

	auto path_more_onetouch = model.FindPath(PageId::System, PageId::MoreOneTouch);
	BOOST_CHECK(path_more_onetouch.empty());

	auto path_custom_label = model.FindPath(PageId::System, PageId::CustomLabel);
	BOOST_CHECK(path_custom_label.empty());
}

BOOST_AUTO_TEST_CASE(TestUnreachablePagesHaveNoPathFromAnyRoot)
{
	// Verify unreachable pages can't be reached from any of the three root pages
	auto model = CreateOneTouchMenuModel();

	std::vector<PageId> roots = { PageId::System, PageId::OneTouch, PageId::EquipmentOnOff };
	std::vector<PageId> unreachable = { PageId::SetAquapure, PageId::MoreOneTouch, PageId::CustomLabel };

	for (auto root : roots)
	{
		for (auto target : unreachable)
		{
			auto path = model.FindPath(root, target);
			BOOST_CHECK_MESSAGE(path.empty(),
				"Unexpected path found from root to unreachable page");
		}
	}
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// Realistic Page Content Registry: "as understood" OneTouch screen layouts
// Content sourced from hardware captures where available, inferred from
// model detector patterns and edge labels otherwise.
// =============================================================================
static std::unordered_map<PageId, ScreenDataPage> CreateOneTouchPageContentRegistry()
{
	std::unordered_map<PageId, ScreenDataPage> registry;

	// --- System (from hardware capture) ---
	{
		ScreenDataPage p(12);
		p[0].Text = "Paddock Pools";
		p[2].Text = " 01/18/11 Tue";
		p[3].Text = "   7:13 PM";
		p[5].Text = "Filter Pump OFF";
		p[6].Text = "    Air 22`C";
		p[9].Text = "Equipment ON/OFF";     // detector + edge
		p[10].Text = "OneTouch  ON/OFF";    // double space (differs from edge label)
		p[11].Text = "   Menu / Help  ";    // spaces around slash (differs from "Menu/Help")
		registry.emplace(PageId::System, std::move(p));
	}

	// --- OneTouch (from hardware capture) ---
	{
		ScreenDataPage p(12);
		p[1].Text = "All Off";
		p[4].Text = "Spa Mode     OFF";
		p[7].Text = "Clean Mode   OFF";
		p[10].Text = " More OneTouch";
		p[11].Text = "     System";          // detector + edge
		registry.emplace(PageId::OneTouch, std::move(p));
	}

	// --- MoreOneTouch (scrolled variant of OneTouch) ---
	{
		ScreenDataPage p(12);
		p[1].Text = "Spillover    OFF";
		p[4].Text = "Pool Mode    OFF";
		p[10].Text = "OneTouch ON/OFF";      // detector
		p[11].Text = "     System";
		registry.emplace(PageId::MoreOneTouch, std::move(p));
	}

	// --- MenuHelp (shifted layout: no SetAquapure, items shift up) ---
	{
		ScreenDataPage p(12);
		p[0].Text = "Menu";                  // detector
		p[1].Text = "Help";                  // edge: HelpSubmenu
		p[2].Text = "Program";               // edge: Program
		p[3].Text = "Set Temp";              // edge: SetTemperature
		p[4].Text = "Set Time";              // edge: SetTime
		p[5].Text = "Display Light";         // shifted from trigger_line 6
		p[6].Text = "Lockouts";              // shifted from trigger_line 7
		p[7].Text = "Password";              // shifted from trigger_line 8
		p[8].Text = "Program Group";         // shifted from trigger_line 9
		p[9].Text = "System Setup";          // shifted from trigger_line 10
		registry.emplace(PageId::MenuHelp, std::move(p));
	}

	// --- HelpSubmenu ---
	{
		ScreenDataPage p(12);
		p[0].Text = "Help";                  // detector 1
		p[1].Text = "Select the item";
		p[2].Text = "you want help";
		p[3].Text = "with.";
		p[5].Text = "Go Back";
		p[7].Text = "Keys";                  // detector 2 + edge
		p[8].Text = "Service";               // edge: HelpService
		p[9].Text = "Diagnostics";           // edge: DiagnosticsSensors
		registry.emplace(PageId::HelpSubmenu, std::move(p));
	}

	// --- HelpKeys ---
	{
		ScreenDataPage p(12);
		p[0].Text = "    Key Help    ";      // detector 1
		p[2].Text = "Select: execute";
		p[3].Text = "Back: go back";
		p[4].Text = "Up/Down: scroll";
		p[11].Text = "    Continue    ";     // detector 2
		registry.emplace(PageId::HelpKeys, std::move(p));
	}

	// --- HelpService ---
	{
		ScreenDataPage p(12);
		p[0].Text = "  Service Help  ";      // detector
		p[2].Text = "Contact your";
		p[3].Text = "dealer for";
		p[4].Text = "service.";
		p[11].Text = "    Continue    ";
		registry.emplace(PageId::HelpService, std::move(p));
	}

	// --- DiagnosticsSensors (from hardware capture) ---
	{
		ScreenDataPage p(12);
		p[0].Text = "Model   B0029221";
		p[1].Text = "Type  RS-8 Combo";
		p[2].Text = "Firmware   T.0.1";
		p[6].Text = "    Sensors";           // detector
		p[7].Text = "Water         OK";
		p[8].Text = "Air           OK";
		p[9].Text = "Solar         OK";
		p[11].Text = "      Next ";          // edge label at line 11 (trigger_line=0)
		registry.emplace(PageId::DiagnosticsSensors, std::move(p));
	}

	// --- DiagnosticsRemotes (from hardware capture) ---
	{
		ScreenDataPage p(12);
		p[0].Text = "    Remotes";           // detector
		p[2].Text = "OneTouch       4";
		p[3].Text = "LX Htr         1";
		p[4].Text = "               1";
		p[5].Text = "(Not Compatible)";
		p[6].Text = "Spa Sw Board   1";
		p[11].Text = "      Next";           // edge label at line 11 (trigger_line=0)
		registry.emplace(PageId::DiagnosticsRemotes, std::move(p));
	}

	// --- DiagnosticsErrors (from hardware capture) ---
	{
		ScreenDataPage p(12);
		p[0].Text = "     Errors";           // detector
		p[5].Text = "   No Errors";
		p[11].Text = "   Continue";          // edge label at line 11 (trigger_line=0)
		registry.emplace(PageId::DiagnosticsErrors, std::move(p));
	}

	// --- DiagnosticsIAQStatus ---
	{
		ScreenDataPage p(12);
		p[0].Text = "iAquaLink Status";      // detector
		p[2].Text = "Connection   OK";
		p[3].Text = "Server      OK";
		p[11].Text = "    Continue    ";     // edge label at line 11 (trigger_line=0)
		registry.emplace(PageId::DiagnosticsIAQStatus, std::move(p));
	}

	// --- DiagnosticsIAQRSSI ---
	{
		ScreenDataPage p(12);
		p[0].Text = "iAquaLink RSSI";        // detector
		p[2].Text = "Signal     -65dB";
		p[11].Text = "    Continue    ";     // edge label at line 11 (trigger_line=0)
		registry.emplace(PageId::DiagnosticsIAQRSSI, std::move(p));
	}

	// --- SystemSetup ---
	{
		ScreenDataPage p(12);
		p[0].Text = "System Setup";          // detector
		p[2].Text = "Label Aux";             // edge: LabelAuxList
		registry.emplace(PageId::SystemSetup, std::move(p));
	}

	// --- LabelAuxList (from hardware capture) ---
	{
		ScreenDataPage p(12);
		p[0].Text = "   Label Aux";          // detector
		p[2].Text = "Aux1           >";
		p[3].Text = "Aux2           >";
		p[4].Text = "Aux3           >";
		p[5].Text = "Aux4           >";
		p[6].Text = "Aux5           >";
		p[7].Text = "Aux6           >";
		p[8].Text = "Aux7           >";
		p[9].Text = "Aux B1         >";
		p[10].Text = "   ^^ More vv";
		registry.emplace(PageId::LabelAuxList, std::move(p));
	}

	// --- LabelAux (from hardware capture) ---
	{
		ScreenDataPage p(12);
		p[0].Text = "   Label Aux1";
		p[2].Text = " Current Label ";       // detector
		p[3].Text = "      Aux1";
		p[5].Text = "General Labels >";      // edge: GeneralLabels
		p[6].Text = "Light   Labels >";      // edge: LightLabels (3-space gap)
		p[7].Text = "Wtrfall Labels >";      // edge: WaterfallLabels (abbreviated)
		p[8].Text = "Custom  Label  >";      // no edge (removed - interactive editor)
		registry.emplace(PageId::LabelAux, std::move(p));
	}

	// --- GeneralLabels ---
	{
		ScreenDataPage p(12);
		p[0].Text = "   Label Aux1";
		p[1].Text = " General Labels";       // detector
		p[3].Text = "Pool";
		p[4].Text = "Spa";
		p[5].Text = "Air Blwr";
		p[6].Text = "Cleaner";
		p[7].Text = "Pool Lo";
		p[8].Text = "Pool Hi";
		registry.emplace(PageId::GeneralLabels, std::move(p));
	}

	// --- LightLabels ---
	{
		ScreenDataPage p(12);
		p[0].Text = "   Label Aux1";
		p[1].Text = "  Light Labels  ";      // detector (1-space; edge label has 3-space)
		p[3].Text = "IntelliBrt";
		p[4].Text = "Light";
		p[5].Text = "SAm Lite";
		registry.emplace(PageId::LightLabels, std::move(p));
	}

	// --- WaterfallLabels ---
	{
		ScreenDataPage p(12);
		p[0].Text = "   Label Aux1";
		p[1].Text = " Wtrfall Labels";       // detector
		p[3].Text = "Waterfall";
		p[4].Text = "Rain";
		registry.emplace(PageId::WaterfallLabels, std::move(p));
	}

	// --- CustomLabel (interactive character editor - not a navigation target) ---
	{
		ScreenDataPage p(12);
		p[0].Text = "   Label Aux1";
		p[1].Text = "      Aux1";
		p[7].Text = "Use Arrow Keys";        // detector
		p[8].Text = "to change letter";
		p[9].Text = "Press SELECT";
		p[10].Text = "to continue.";
		registry.emplace(PageId::CustomLabel, std::move(p));
	}

	// --- EquipmentOnOff (from hardware capture) ---
	{
		ScreenDataPage p(12);
		p[0].Text = "Filter Pump  ***";      // detector
		p[1].Text = "Spa           ON";
		p[2].Text = "Pool Heat    ENA";
		p[3].Text = "Spa Heat     OFF";
		p[4].Text = "Solar Heat   OFF";
		p[5].Text = "Aux1         OFF";
		p[6].Text = "Aux2         OFF";
		p[7].Text = "Aux3         OFF";
		p[8].Text = "Aux4         OFF";
		p[9].Text = "Aux5         OFF";
		p[10].Text = "Aux6         OFF";
		p[11].Text = "   ^^ More vv";       // "System" not visible (requires scroll)
		registry.emplace(PageId::EquipmentOnOff, std::move(p));
	}

	// --- EquipmentStatus ---
	{
		ScreenDataPage p(12);
		p[0].Text = "EQUIPMENT STATUS";      // detector (uppercase)
		p[2].Text = "Intelliflo VS 3";
		p[3].Text = "   RPM: 2750";
		p[4].Text = "    Watts: 600";
		p[5].Text = "      GPM: 55";
		registry.emplace(PageId::EquipmentStatus, std::move(p));
	}

	// --- SetTemperature (from hardware capture) ---
	{
		ScreenDataPage p(12);
		p[0].Text = "    Set Temp";          // detector
		p[2].Text = "Pool Heat   90`F";
		p[3].Text = "Spa Heat   102`F";
		p[5].Text = "Maintain     OFF";
		p[6].Text = "Hours  12AM-12AM";
		p[8].Text = "Highlight an";
		p[9].Text = "item and press";
		p[10].Text = "Select";
		registry.emplace(PageId::SetTemperature, std::move(p));
	}

	// --- SetTime ---
	{
		ScreenDataPage p(12);
		p[0].Text = "    Set Time";          // detector 1
		p[2].Text = " 01/18/11 Tue";
		p[3].Text = "   7:13 PM";
		p[7].Text = "Use Arrow Keys";        // detector 2
		p[8].Text = "to set value.";
		p[9].Text = "Press SELECT";
		p[10].Text = "to continue.";
		registry.emplace(PageId::SetTime, std::move(p));
	}

	// --- FreezeProtect (from hardware capture) ---
	{
		ScreenDataPage p(12);
		p[0].Text = " Freeze Protect";       // detector
		p[3].Text = "Temp        38`F";
		p[6].Text = "Use Arrow Keys";
		p[7].Text = "to set value.";
		p[8].Text = "Press SELECT";
		p[9].Text = "to continue.";
		registry.emplace(PageId::FreezeProtect, std::move(p));
	}

	// --- Boost ---
	{
		ScreenDataPage p(12);
		p[0].Text = "   Boost Pool";         // detector
		p[3].Text = "Pool Boost OFF";
		registry.emplace(PageId::Boost, std::move(p));
	}

	// --- SetAquapure ---
	{
		ScreenDataPage p(12);
		p[0].Text = "  Set AquaPure";        // detector
		p[3].Text = "AquaPure   50%";
		registry.emplace(PageId::SetAquapure, std::move(p));
	}

	// --- SelectSpeed ---
	{
		ScreenDataPage p(12);
		p[0].Text = "  Select Speed";        // detector
		p[3].Text = "Low      1100";
		p[4].Text = "Medium   2200";
		p[5].Text = "High     3100";
		registry.emplace(PageId::SelectSpeed, std::move(p));
	}

	// --- Program ---
	{
		ScreenDataPage p(12);
		p[0].Text = "    Program";           // detector
		p[2].Text = "Filter:  On  Off";
		p[3].Text = "1)  4:30A 10:00A";
		p[4].Text = "2) 10:30A  4:00P";
		p[5].Text = "Spa:     On  Off";
		registry.emplace(PageId::Program, std::move(p));
	}

	// --- DisplayLight ---
	{
		ScreenDataPage p(12);
		p[0].Text = " Display Light";        // detector
		p[3].Text = "Light Mode  OFF";
		registry.emplace(PageId::DisplayLight, std::move(p));
	}

	// --- Lockouts ---
	{
		ScreenDataPage p(12);
		p[0].Text = "    Lockout";           // detector (no 's')
		p[3].Text = "Panel Lock  OFF";
		registry.emplace(PageId::Lockouts, std::move(p));
	}

	// --- PasswordSettings ---
	{
		ScreenDataPage p(12);
		p[0].Text = "    Password";          // detector
		p[3].Text = "Password   OFF";
		registry.emplace(PageId::PasswordSettings, std::move(p));
	}

	// --- ProgramGroup ---
	{
		ScreenDataPage p(12);
		p[0].Text = " Program Group";        // detector
		p[3].Text = "Group 1";
		p[4].Text = "Group 2";
		p[5].Text = "Group 3";
		registry.emplace(PageId::ProgramGroup, std::move(p));
	}

	// --- StartUp (cold start splash) ---
	// max_content_lines=4 means this page only matches if <=4 non-empty lines
	{
		ScreenDataPage p(12);
		p[4].Text = "    B0029221";
		p[5].Text = "   RS-8 Combo";         // detector 2 (contains "-")
		p[7].Text = "   REV T.0.1";          // detector 1 (starts with "REV ")
		registry.emplace(PageId::StartUp, std::move(p));
	}

	// --- Service (from hardware capture) ---
	{
		ScreenDataPage p(12);
		p[3].Text = " Service Mode";         // detector
		p[5].Text = "No  operations";
		p[6].Text = " allowed here";
		registry.emplace(PageId::Service, std::move(p));
	}

	// --- TimeOut (from hardware capture) ---
	{
		ScreenDataPage p(12);
		p[3].Text = " Timeout Mode";         // detector
		p[5].Text = "No  operations";
		p[6].Text = " allowed here";
		p[10].Text = "   02:57:39";
		registry.emplace(PageId::TimeOut, std::move(p));
	}

	// --- EnterPassword ---
	{
		ScreenDataPage p(12);
		p[0].Text = " Enter Password";       // detector
		p[3].Text = "_ _ _ _";
		registry.emplace(PageId::EnterPassword, std::move(p));
	}

	return registry;
}

// =============================================================================
// VirtualController: simulates OneTouch AquaLink controller for test crawling
// =============================================================================
class VirtualController
{
public:
	VirtualController(
		const MenuModel& model,
		const std::unordered_map<PageId, ScreenDataPage>& registry,
		PageId start_page)
		: m_Model(model)
		, m_Registry(registry)
		, m_CurrentPage(start_page)
		, m_Cursor(0)
	{
	}

	void Execute(NavKeyCommand cmd)
	{
		const MenuPage* page = m_Model.GetPage(m_CurrentPage);
		if (!page) return;

		switch (cmd)
		{
		case NavKeyCommand::Select:
		{
			// Primary: match by content label at cursor position
			// This mirrors real hardware: Select activates whatever item is
			// at the cursor position, regardless of model trigger_lines.
			// Critical for shifted menus (e.g., MenuHelp without SetAquapure).
			auto it = m_Registry.find(m_CurrentPage);
			if (it != m_Registry.end() && m_Cursor < it->second.Size())
			{
				auto text = it->second[m_Cursor].Text;
				auto start = text.find_first_not_of(" \t");
				if (start != std::string::npos)
				{
					auto trimmed = text.substr(start);
					for (const auto& edge : page->edges)
					{
						if (edge.trigger == EdgeTrigger::Select && !edge.label.empty())
						{
							if (trimmed.starts_with(edge.label))
							{
								m_CurrentPage = edge.target;
								m_Cursor = 0;
								return;
							}
						}
					}
				}
			}
			// Fallback: match by trigger_line == cursor position
			// For pages where content doesn't match edge labels (e.g.,
			// EquipmentOnOff with "^^ More vv" at line 11 instead of "System")
			for (const auto& edge : page->edges)
			{
				if (edge.trigger == EdgeTrigger::Select && edge.trigger_line == m_Cursor)
				{
					m_CurrentPage = edge.target;
					m_Cursor = 0;
					return;
				}
			}
			break;  // No match - equipment toggle, no page change
		}
		case NavKeyCommand::Back:
		{
			for (const auto& edge : page->edges)
			{
				if (edge.trigger == EdgeTrigger::Back)
				{
					m_CurrentPage = edge.target;
					m_Cursor = 0;
					return;
				}
			}
			break;
		}
		case NavKeyCommand::LineUp:
			if (m_Cursor > 0) m_Cursor--;
			break;
		case NavKeyCommand::LineDown:
			if (m_Cursor < 11) m_Cursor++;
			break;
		case NavKeyCommand::PageUp_Or_Select3:
		case NavKeyCommand::PageDown_Or_Select1:
		default:
			break;  // Scroll/other - no-op in test
		}
	}

	const ScreenDataPage& GetContent() const
	{
		static ScreenDataPage empty(12);
		auto it = m_Registry.find(m_CurrentPage);
		return (it != m_Registry.end()) ? it->second : empty;
	}

	uint8_t GetCursor() const { return m_Cursor; }
	PageId GetCurrentPage() const { return m_CurrentPage; }

private:
	const MenuModel& m_Model;
	const std::unordered_map<PageId, ScreenDataPage>& m_Registry;
	PageId m_CurrentPage;
	uint8_t m_Cursor;
};

// =============================================================================
// Helper: determine number of status messages for a given command
// =============================================================================
static int StatusMessageCount(NavKeyCommand cmd)
{
	switch (cmd)
	{
	case NavKeyCommand::Select:
	case NavKeyCommand::Back:
		return 2;  // Page transitions need 2 status messages
	case NavKeyCommand::LineUp:
	case NavKeyCommand::LineDown:
	case NavKeyCommand::PageUp_Or_Select3:
	case NavKeyCommand::PageDown_Or_Select1:
		return 1;  // Cursor/scroll moves need 1
	default:
		return 0;
	}
}

// =============================================================================
// RealisticDetection_TestSuite
// =============================================================================
BOOST_AUTO_TEST_SUITE(RealisticDetection_TestSuite)

BOOST_AUTO_TEST_CASE(TestDetectPageMatchesAllRealisticContent)
{
	auto model = CreateOneTouchMenuModel();
	auto registry = CreateOneTouchPageContentRegistry();

	// Every page in the registry should be correctly detected by DetectPage
	for (const auto& [page_id, content] : registry)
	{
		if (page_id == PageId::Unknown) continue;

		PageId detected = model.DetectPage(content);
		BOOST_CHECK_MESSAGE(detected == page_id,
			"DetectPage mismatch for page " << static_cast<uint32_t>(page_id)
			<< ": expected " << static_cast<uint32_t>(page_id)
			<< " but got " << static_cast<uint32_t>(detected));
	}
}

BOOST_AUTO_TEST_CASE(TestNoCrossDetectionBetweenPages)
{
	auto model = CreateOneTouchMenuModel();
	auto registry = CreateOneTouchPageContentRegistry();

	// For each page's content, DetectPage should return exactly that page (not another)
	// This protects against detector collisions between pages
	uint32_t collision_count = 0;
	for (const auto& [page_id, content] : registry)
	{
		if (page_id == PageId::Unknown) continue;

		PageId detected = model.DetectPage(content);
		if (detected != page_id)
		{
			collision_count++;
		}
	}
	BOOST_CHECK_EQUAL(collision_count, 0);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// EndToEndScrape_TestSuite
// =============================================================================
BOOST_AUTO_TEST_SUITE(EndToEndScrape_TestSuite)

BOOST_AUTO_TEST_CASE(TestFullDiscoveryCrawlVisits25Pages)
{
	auto model = CreateOneTouchMenuModel();
	auto registry = CreateOneTouchPageContentRegistry();
	Navigator nav(model);
	SpiderEngine engine(model, nav);

	std::vector<PageId> captured;
	bool crawl_complete = false;

	auto policy = std::make_unique<FullDiscoveryVisitPolicy>(
		[&](PageId page, const ScreenDataPage& content) { captured.push_back(page); },
		[&]() { crawl_complete = true; }
	);
	engine.StartCrawl(std::move(policy));

	VirtualController controller(model, registry, PageId::System);

	// Run the crawl loop
	const uint32_t MAX_ITERATIONS = 5000;
	uint32_t iterations = 0;

	while (engine.GetState() != SpiderEngine::State::Complete &&
	       engine.GetState() != SpiderEngine::State::Failed &&
	       iterations < MAX_ITERATIONS)
	{
		iterations++;
		const auto& content = controller.GetContent();
		auto cursor = controller.GetCursor();

		auto cmd = engine.ProcessStep(content, cursor);

		if (cmd.has_value())
		{
			int status_count = StatusMessageCount(cmd.value());
			for (int i = 0; i < status_count; i++)
			{
				nav.OnStatusMessageReceived();
			}
			controller.Execute(cmd.value());
		}
	}

	// Verify completion
	BOOST_CHECK_MESSAGE(engine.GetState() == SpiderEngine::State::Complete,
		"Spider engine did not complete: state=" << static_cast<int>(engine.GetState())
		<< " after " << iterations << " iterations");

	BOOST_CHECK(crawl_complete);

	// Verify all 25 reachable pages were visited
	const auto& visited = engine.GetVisitedPages();
	BOOST_CHECK_EQUAL(visited.size(), 25);

	// Check all expected reachable pages
	std::vector<PageId> expected_reachable = {
		PageId::System, PageId::OneTouch, PageId::EquipmentOnOff,
		PageId::MenuHelp, PageId::HelpSubmenu, PageId::HelpKeys, PageId::HelpService,
		PageId::DiagnosticsSensors, PageId::DiagnosticsRemotes, PageId::DiagnosticsErrors,
		PageId::DiagnosticsIAQStatus, PageId::DiagnosticsIAQRSSI,
		PageId::Program, PageId::SetTemperature, PageId::SetTime,
		PageId::DisplayLight, PageId::Lockouts, PageId::PasswordSettings,
		PageId::ProgramGroup, PageId::SystemSetup,
		PageId::LabelAuxList, PageId::LabelAux,
		PageId::GeneralLabels, PageId::LightLabels, PageId::WaterfallLabels
	};

	for (PageId expected : expected_reachable)
	{
		BOOST_CHECK_MESSAGE(visited.count(expected) > 0,
			"Expected page " << static_cast<uint32_t>(expected) << " was not visited");
	}

	// Verify unreachable pages were NOT visited
	std::vector<PageId> unreachable = {
		PageId::SetAquapure, PageId::MoreOneTouch, PageId::CustomLabel,
		PageId::EquipmentStatus, PageId::FreezeProtect, PageId::Boost,
		PageId::SelectSpeed
	};

	for (PageId page : unreachable)
	{
		BOOST_CHECK_MESSAGE(visited.count(page) == 0,
			"Unreachable page " << static_cast<uint32_t>(page) << " was unexpectedly visited");
	}
}

BOOST_AUTO_TEST_CASE(TestTargetedCrawlVisitsSubset)
{
	auto model = CreateOneTouchMenuModel();
	auto registry = CreateOneTouchPageContentRegistry();
	Navigator nav(model);
	SpiderEngine engine(model, nav);

	std::set<PageId> targets = {
		PageId::System, PageId::MenuHelp, PageId::SetTemperature,
		PageId::DiagnosticsSensors, PageId::LabelAux
	};

	std::vector<PageId> captured;
	auto policy = std::make_unique<TargetedVisitPolicy>(
		targets,
		[&](PageId page, const ScreenDataPage& content) { captured.push_back(page); },
		nullptr
	);
	engine.StartCrawl(std::move(policy));

	VirtualController controller(model, registry, PageId::System);

	const uint32_t MAX_ITERATIONS = 5000;
	uint32_t iterations = 0;

	while (engine.GetState() != SpiderEngine::State::Complete &&
	       engine.GetState() != SpiderEngine::State::Failed &&
	       iterations < MAX_ITERATIONS)
	{
		iterations++;
		const auto& content = controller.GetContent();
		auto cursor = controller.GetCursor();

		auto cmd = engine.ProcessStep(content, cursor);

		if (cmd.has_value())
		{
			int status_count = StatusMessageCount(cmd.value());
			for (int i = 0; i < status_count; i++)
			{
				nav.OnStatusMessageReceived();
			}
			controller.Execute(cmd.value());
		}
	}

	BOOST_CHECK(engine.GetState() == SpiderEngine::State::Complete);

	// Verify exactly the targeted pages were visited
	const auto& visited = engine.GetVisitedPages();
	BOOST_CHECK_EQUAL(visited.size(), targets.size());

	for (PageId target : targets)
	{
		BOOST_CHECK_MESSAGE(visited.count(target) > 0,
			"Target page " << static_cast<uint32_t>(target) << " was not visited");
	}
}

BOOST_AUTO_TEST_CASE(TestCrawlFromEquipmentOnOff)
{
	auto model = CreateOneTouchMenuModel();
	auto registry = CreateOneTouchPageContentRegistry();
	Navigator nav(model);

	// Sync navigator to EquipmentOnOff before starting crawl
	nav.StartSync();
	auto eq_content = MakePageContent(model, PageId::EquipmentOnOff);
	for (uint32_t i = 0; i < Navigator::SYNC_REQUIRED_CONSISTENT_COUNT + 1; ++i)
	{
		nav.OnPageUpdate(eq_content, 0);
	}
	BOOST_REQUIRE(nav.IsSynced());
	BOOST_REQUIRE(nav.GetCurrentPage() == PageId::EquipmentOnOff);

	SpiderEngine engine(model, nav);

	std::vector<PageId> captured;
	auto policy = std::make_unique<FullDiscoveryVisitPolicy>(
		[&](PageId page, const ScreenDataPage& content) { captured.push_back(page); },
		nullptr
	);
	engine.StartCrawl(std::move(policy));

	VirtualController controller(model, registry, PageId::EquipmentOnOff);

	const uint32_t MAX_ITERATIONS = 5000;
	uint32_t iterations = 0;

	while (engine.GetState() != SpiderEngine::State::Complete &&
	       engine.GetState() != SpiderEngine::State::Failed &&
	       iterations < MAX_ITERATIONS)
	{
		iterations++;
		const auto& content = controller.GetContent();
		auto cursor = controller.GetCursor();

		auto cmd = engine.ProcessStep(content, cursor);

		if (cmd.has_value())
		{
			int status_count = StatusMessageCount(cmd.value());
			for (int i = 0; i < status_count; i++)
			{
				nav.OnStatusMessageReceived();
			}
			controller.Execute(cmd.value());
		}
	}

	BOOST_CHECK_MESSAGE(engine.GetState() == SpiderEngine::State::Complete,
		"Spider engine starting from EquipmentOnOff did not complete: state="
		<< static_cast<int>(engine.GetState()) << " after " << iterations << " iterations");

	// Should still visit all 25 reachable pages
	const auto& visited = engine.GetVisitedPages();
	BOOST_CHECK_EQUAL(visited.size(), 25);
}

BOOST_AUTO_TEST_SUITE_END()
