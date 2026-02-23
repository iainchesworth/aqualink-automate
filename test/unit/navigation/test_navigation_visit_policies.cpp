#include <set>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "navigation/menu_model.h"
#include "navigation/visit_policies.h"
#include "utility/screen_data_page.h"

using namespace AqualinkAutomate::Navigation;
using namespace AqualinkAutomate::Utility;

BOOST_AUTO_TEST_SUITE(Navigation_VisitPoliciesTestSuite)

// =============================================================================
// TargetedVisitPolicy: ShouldVisit
// =============================================================================

BOOST_AUTO_TEST_CASE(TestTargeted_ShouldVisit_TrueForTargetPages)
{
	std::set<PageId> targets = { PageId::System, PageId::EquipmentOnOff };

	TargetedVisitPolicy policy(targets);

	MenuPage system_page;
	system_page.id = PageId::System;
	system_page.name = "System";
	BOOST_CHECK(policy.ShouldVisit(PageId::System, system_page));

	MenuPage equip_page;
	equip_page.id = PageId::EquipmentOnOff;
	equip_page.name = "EquipmentOnOff";
	BOOST_CHECK(policy.ShouldVisit(PageId::EquipmentOnOff, equip_page));
}

BOOST_AUTO_TEST_CASE(TestTargeted_ShouldVisit_FalseForOtherPages)
{
	std::set<PageId> targets = { PageId::System };

	TargetedVisitPolicy policy(targets);

	MenuPage other_page;
	other_page.id = PageId::MenuHelp;
	other_page.name = "MenuHelp";
	BOOST_CHECK(!policy.ShouldVisit(PageId::MenuHelp, other_page));
}

BOOST_AUTO_TEST_CASE(TestTargeted_ShouldVisit_EmptyTargetSet)
{
	std::set<PageId> targets = {};

	TargetedVisitPolicy policy(targets);

	MenuPage page;
	page.id = PageId::System;
	BOOST_CHECK(!policy.ShouldVisit(PageId::System, page));
}

// =============================================================================
// TargetedVisitPolicy: Callbacks
// =============================================================================

BOOST_AUTO_TEST_CASE(TestTargeted_OnPageReached_InvokesCallback)
{
	bool callback_invoked = false;
	PageId received_page = PageId::Unknown;

	auto on_page = [&](PageId page, const ScreenDataPage& content)
	{
		callback_invoked = true;
		received_page = page;
	};

	std::set<PageId> targets = { PageId::System };
	TargetedVisitPolicy policy(targets, on_page);

	ScreenDataPage content(4);
	policy.OnPageReached(PageId::System, content);

	BOOST_CHECK(callback_invoked);
	BOOST_CHECK(received_page == PageId::System);
}

BOOST_AUTO_TEST_CASE(TestTargeted_OnCrawlComplete_InvokesCallback)
{
	bool complete_invoked = false;

	auto on_complete = [&]()
	{
		complete_invoked = true;
	};

	std::set<PageId> targets = { PageId::System };
	TargetedVisitPolicy policy(targets, nullptr, on_complete);

	policy.OnCrawlComplete();

	BOOST_CHECK(complete_invoked);
}

// =============================================================================
// Null Callbacks: No Crash
// =============================================================================

BOOST_AUTO_TEST_CASE(TestTargeted_NullCallbacks_NoCrash)
{
	std::set<PageId> targets = { PageId::System };
	TargetedVisitPolicy policy(targets, nullptr, nullptr);

	ScreenDataPage content(4);
	BOOST_CHECK_NO_THROW(policy.OnPageReached(PageId::System, content));
	BOOST_CHECK_NO_THROW(policy.OnCrawlComplete());
}

// =============================================================================
// FullDiscoveryVisitPolicy: Skipped Pages
// =============================================================================

BOOST_AUTO_TEST_CASE(TestFullDiscovery_SkipsSpecialPages)
{
	FullDiscoveryVisitPolicy policy;

	MenuPage page;

	page.id = PageId::Unknown;
	BOOST_CHECK(!policy.ShouldVisit(PageId::Unknown, page));

	page.id = PageId::StartUp;
	BOOST_CHECK(!policy.ShouldVisit(PageId::StartUp, page));

	page.id = PageId::Service;
	BOOST_CHECK(!policy.ShouldVisit(PageId::Service, page));

	page.id = PageId::TimeOut;
	BOOST_CHECK(!policy.ShouldVisit(PageId::TimeOut, page));

	page.id = PageId::EnterPassword;
	BOOST_CHECK(!policy.ShouldVisit(PageId::EnterPassword, page));
}

BOOST_AUTO_TEST_CASE(TestFullDiscovery_VisitsNormalPages)
{
	FullDiscoveryVisitPolicy policy;

	MenuPage page;

	page.id = PageId::System;
	BOOST_CHECK(policy.ShouldVisit(PageId::System, page));

	page.id = PageId::EquipmentOnOff;
	BOOST_CHECK(policy.ShouldVisit(PageId::EquipmentOnOff, page));

	page.id = PageId::MenuHelp;
	BOOST_CHECK(policy.ShouldVisit(PageId::MenuHelp, page));

	page.id = PageId::SetTemperature;
	BOOST_CHECK(policy.ShouldVisit(PageId::SetTemperature, page));
}

// =============================================================================
// FullDiscoveryVisitPolicy: Callbacks
// =============================================================================

BOOST_AUTO_TEST_CASE(TestFullDiscovery_OnPageReached_InvokesCallback)
{
	bool callback_invoked = false;

	auto on_page = [&](PageId page, const ScreenDataPage& content)
	{
		callback_invoked = true;
	};

	FullDiscoveryVisitPolicy policy(on_page);

	ScreenDataPage content(4);
	policy.OnPageReached(PageId::System, content);

	BOOST_CHECK(callback_invoked);
}

BOOST_AUTO_TEST_CASE(TestFullDiscovery_OnCrawlComplete_InvokesCallback)
{
	bool complete_invoked = false;

	auto on_complete = [&]()
	{
		complete_invoked = true;
	};

	FullDiscoveryVisitPolicy policy(nullptr, on_complete);

	policy.OnCrawlComplete();

	BOOST_CHECK(complete_invoked);
}

BOOST_AUTO_TEST_CASE(TestFullDiscovery_NullCallbacks_NoCrash)
{
	FullDiscoveryVisitPolicy policy(nullptr, nullptr);

	ScreenDataPage content(4);
	BOOST_CHECK_NO_THROW(policy.OnPageReached(PageId::System, content));
	BOOST_CHECK_NO_THROW(policy.OnCrawlComplete());
}

BOOST_AUTO_TEST_SUITE_END()
