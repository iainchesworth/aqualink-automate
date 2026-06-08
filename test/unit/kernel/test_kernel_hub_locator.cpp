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

	// A derived hub used to verify exact-static-type keying: registering it under
	// its own type must not satisfy a Find<> for its base interface and vice versa.
	class DerivedHub : public TestHubA
	{
	public:
		int extra{ 7 };
	};

	// A standalone polymorphic service interface that does NOT derive from IHub,
	// mirroring ICommandDispatcher / IRecordingController which are registered in
	// the same locator alongside the IHub state hubs.
	class TestService
	{
	public:
		virtual ~TestService() = default;
		int token{ 99 };
	};
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

// =============================================================================
// Exact-static-type keying (resolution is by typeid(T), not by base interface)
// =============================================================================

BOOST_AUTO_TEST_CASE(TestRegister_ResolutionIsByExactStaticType)
{
	HubLocator locator;
	auto derived = std::make_shared<DerivedHub>();
	locator.Register(derived);

	// Registered as DerivedHub -> found as DerivedHub.
	auto found_derived = locator.TryFind<DerivedHub>();
	BOOST_CHECK(found_derived != nullptr);
	BOOST_CHECK_EQUAL(found_derived->extra, 7);

	// NOT resolvable via the base interfaces it derives from.
	BOOST_CHECK(locator.TryFind<TestHubA>() == nullptr);
	BOOST_CHECK(locator.TryFind<IHub>() == nullptr);
}

BOOST_AUTO_TEST_CASE(TestRegister_BaseAndDerivedAreDistinctKeys)
{
	HubLocator locator;
	auto base = std::make_shared<TestHubA>();
	base->name = "Base";
	auto derived = std::make_shared<DerivedHub>();
	derived->name = "Derived";

	locator.Register(base);
	locator.Register(derived);

	// Both keys coexist independently.
	BOOST_CHECK_EQUAL(locator.Find<TestHubA>()->name, "Base");
	BOOST_CHECK_EQUAL(locator.Find<DerivedHub>()->name, "Derived");
}

// =============================================================================
// Re-register overwrites the existing handle for the same type (no leak/dup)
// =============================================================================

BOOST_AUTO_TEST_CASE(TestRegister_SameType_OverwritesWithoutUnregister)
{
	HubLocator locator;
	auto first = std::make_shared<TestHubA>();
	first->name = "First";
	locator.Register(first);

	auto second = std::make_shared<TestHubA>();
	second->name = "Second";
	locator.Register(second);

	BOOST_CHECK_EQUAL(locator.Find<TestHubA>()->name, "Second");
}

// =============================================================================
// Standalone (non-IHub) service interfaces register/resolve like hubs
// =============================================================================

BOOST_AUTO_TEST_CASE(TestRegister_NonHubServiceInterface_ResolvesAndIsDistinct)
{
	HubLocator locator;
	auto service = std::make_shared<TestService>();
	auto hub = std::make_shared<TestHubA>();

	locator.Register(service).Register(hub);

	auto found_service = locator.TryFind<TestService>();
	BOOST_REQUIRE(found_service != nullptr);
	BOOST_CHECK_EQUAL(found_service->token, 99);

	// The service handle round-trips through the shared_ptr<void> erasure intact.
	BOOST_CHECK(found_service.get() == service.get());

	// Distinct key from the IHub-derived hub.
	BOOST_CHECK(locator.TryFind<TestHubA>() != nullptr);

	locator.Unregister<TestService>();
	BOOST_CHECK(locator.TryFind<TestService>() == nullptr);
	BOOST_CHECK(locator.TryFind<TestHubA>() != nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
