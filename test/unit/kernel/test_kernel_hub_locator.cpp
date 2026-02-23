#include <memory>
#include <string>

#include <boost/test/unit_test.hpp>

#include "exceptions/exception_hubnotfound.h"
#include "interfaces/ihub.h"
#include "kernel/hub_locator.h"

using namespace AqualinkAutomate::Kernel;
using namespace AqualinkAutomate::Interfaces;
using namespace AqualinkAutomate::Exceptions;

namespace
{
	class TestHubA : public IHub
	{
	public:
		std::string name{ "HubA" };
	};

	class TestHubB : public IHub
	{
	public:
		int value{ 42 };
	};

	class TestHubC : public IHub {};
}

BOOST_AUTO_TEST_SUITE(HubLocator_TestSuite)

// =============================================================================
// Register and Find
// =============================================================================

BOOST_AUTO_TEST_CASE(TestRegister_ThenFind_ReturnsHub)
{
	HubLocator locator;
	auto hub = std::make_shared<TestHubA>();
	locator.Register(hub);

	auto found = locator.Find<TestHubA>();
	BOOST_CHECK(found != nullptr);
	BOOST_CHECK_EQUAL(found->name, "HubA");
}

BOOST_AUTO_TEST_CASE(TestRegister_MultipleHubs_FindsEach)
{
	HubLocator locator;
	auto hubA = std::make_shared<TestHubA>();
	auto hubB = std::make_shared<TestHubB>();

	locator.Register(hubA).Register(hubB);

	auto foundA = locator.Find<TestHubA>();
	auto foundB = locator.Find<TestHubB>();
	BOOST_CHECK(foundA != nullptr);
	BOOST_CHECK(foundB != nullptr);
	BOOST_CHECK_EQUAL(foundA->name, "HubA");
	BOOST_CHECK_EQUAL(foundB->value, 42);
}

// =============================================================================
// Find throws when not registered
// =============================================================================

BOOST_AUTO_TEST_CASE(TestFind_NotRegistered_Throws)
{
	HubLocator locator;
	BOOST_CHECK_THROW(locator.Find<TestHubA>(), Hub_NotFound);
}

// =============================================================================
// TryFind
// =============================================================================

BOOST_AUTO_TEST_CASE(TestTryFind_Registered_ReturnsHub)
{
	HubLocator locator;
	auto hub = std::make_shared<TestHubA>();
	locator.Register(hub);

	auto found = locator.TryFind<TestHubA>();
	BOOST_CHECK(found != nullptr);
	BOOST_CHECK_EQUAL(found->name, "HubA");
}

BOOST_AUTO_TEST_CASE(TestTryFind_NotRegistered_ReturnsNullptr)
{
	HubLocator locator;
	auto found = locator.TryFind<TestHubA>();
	BOOST_CHECK(found == nullptr);
}

// =============================================================================
// Unregister
// =============================================================================

BOOST_AUTO_TEST_CASE(TestUnregister_Registered_RemovesHub)
{
	HubLocator locator;
	auto hub = std::make_shared<TestHubA>();
	locator.Register(hub);
	BOOST_REQUIRE(locator.TryFind<TestHubA>() != nullptr);

	locator.Unregister<TestHubA>();
	BOOST_CHECK(locator.TryFind<TestHubA>() == nullptr);
}

BOOST_AUTO_TEST_CASE(TestUnregister_NotRegistered_DoesNotThrow)
{
	HubLocator locator;
	BOOST_CHECK_NO_THROW(locator.Unregister<TestHubA>());
}

BOOST_AUTO_TEST_CASE(TestUnregister_OnlyRemovesMatchingType)
{
	HubLocator locator;
	auto hubA = std::make_shared<TestHubA>();
	auto hubB = std::make_shared<TestHubB>();
	locator.Register(hubA).Register(hubB);

	locator.Unregister<TestHubA>();
	BOOST_CHECK(locator.TryFind<TestHubA>() == nullptr);
	BOOST_CHECK(locator.TryFind<TestHubB>() != nullptr);
}

BOOST_AUTO_TEST_CASE(TestUnregister_ThenReRegister_Works)
{
	HubLocator locator;
	auto hub1 = std::make_shared<TestHubA>();
	hub1->name = "First";
	locator.Register(hub1);

	locator.Unregister<TestHubA>();

	auto hub2 = std::make_shared<TestHubA>();
	hub2->name = "Second";
	locator.Register(hub2);

	auto found = locator.Find<TestHubA>();
	BOOST_CHECK_EQUAL(found->name, "Second");
}

// =============================================================================
// Chaining
// =============================================================================

BOOST_AUTO_TEST_CASE(TestChaining_RegisterReturnsSelf)
{
	HubLocator locator;
	auto hubA = std::make_shared<TestHubA>();
	auto hubB = std::make_shared<TestHubB>();

	locator.Register(hubA).Register(hubB);

	BOOST_CHECK(locator.TryFind<TestHubA>() != nullptr);
	BOOST_CHECK(locator.TryFind<TestHubB>() != nullptr);
}

BOOST_AUTO_TEST_CASE(TestChaining_UnregisterReturnsSelf)
{
	HubLocator locator;
	auto hubA = std::make_shared<TestHubA>();
	auto hubB = std::make_shared<TestHubB>();
	locator.Register(hubA).Register(hubB);

	locator.Unregister<TestHubA>().Unregister<TestHubB>();
	BOOST_CHECK(locator.TryFind<TestHubA>() == nullptr);
	BOOST_CHECK(locator.TryFind<TestHubB>() == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
