#include <memory>
#include <string>

#include <boost/test/unit_test.hpp>

#include "profiling/profiling_controller.h"
#include "profiling/factories/profiler_factory.h"
#include "profiling/noop_profiler.h"
#include "profiling/types/profiling_types.h"

using namespace AqualinkAutomate;

// =============================================================================
// ProfilingController — the runtime control surface over the ProfilerFactory.
//
// The factory is a process-wide singleton; this test process never calls
// RegisterAvailableProfilers, so most assertions are written as RELATIONSHIPS
// against the live factory state (enabled == any registered, etc.) to stay
// robust to whatever singleton state other test cases have set. One case
// registers a backend to exercise the positive resume/pause toggle.
// =============================================================================

BOOST_AUTO_TEST_SUITE(ProfilingController_TestSuite)

BOOST_AUTO_TEST_CASE(ProfilingController_Status_IsWellFormedAndMatchesFactory)
{
	auto& factory = Factory::ProfilerFactory::Instance();
	Profiling::ProfilingController controller;

	Interfaces::IProfilingController::Status status;
	BOOST_CHECK_NO_THROW(status = controller.ProfilingStatus());

	const auto registered_count = factory.RegisteredTypes().size();
	BOOST_CHECK_EQUAL(status.enabled, registered_count != 0);
	BOOST_CHECK_EQUAL(status.available_backends.size(), registered_count);

	if (!status.enabled)
	{
		BOOST_CHECK(!status.running);
		BOOST_CHECK(status.active_backend.empty());
	}
}

BOOST_AUTO_TEST_CASE(ProfilingController_StartStop_AreSafe)
{
	auto& factory = Factory::ProfilerFactory::Instance();
	Profiling::ProfilingController controller;

	// Always safe to call; only succeed when a backend is actually active.
	bool started = false;
	bool stopped = false;
	BOOST_CHECK_NO_THROW(started = controller.Start());
	BOOST_CHECK_NO_THROW(stopped = controller.Stop());

	const bool has_active = factory.Selected().has_value() && factory.IsRegistered(*factory.Selected());
	if (!has_active)
	{
		BOOST_CHECK(!started);
		BOOST_CHECK(!stopped);
	}
}

BOOST_AUTO_TEST_CASE(ProfilingController_SelectBackend_RejectsUnknownName)
{
	Profiling::ProfilingController controller;
	BOOST_CHECK(!controller.SelectBackend("not-a-real-backend"));
}

BOOST_AUTO_TEST_CASE(ProfilingController_SelectBackend_MatchesRegistration)
{
	auto& factory = Factory::ProfilerFactory::Instance();
	Profiling::ProfilingController controller;

	// A recognised name only succeeds when that backend was compiled into the
	// build (i.e. registered) — so the result must equal IsRegistered().
	BOOST_CHECK_EQUAL(controller.SelectBackend("tracy"), factory.IsRegistered(Types::ProfilerTypes::Tracy));
	// Case-insensitive parsing must agree with the lowercase form.
	BOOST_CHECK_EQUAL(controller.SelectBackend("TRACY"), factory.IsRegistered(Types::ProfilerTypes::Tracy));
}

// Registers a backend so the positive resume/pause toggle can be exercised. The
// registration is process-global (the factory has no Unregister) but harmless:
// the other cases assert relationships against the live factory state, so they
// stay correct whether or not a backend is present.
BOOST_AUTO_TEST_CASE(ProfilingController_StartStop_TogglesRunning_WhenBackendActive)
{
	auto& factory = Factory::ProfilerFactory::Instance();
	factory.Register(Types::ProfilerTypes::Tracy, std::make_shared<Profiling::NoOp_Profiler>());
	factory.SetDesired(Types::ProfilerTypes::Tracy);

	Profiling::ProfilingController controller;

	auto status = controller.ProfilingStatus();
	BOOST_REQUIRE(status.enabled);
	BOOST_CHECK_EQUAL(status.active_backend, "tracy");

	BOOST_CHECK(controller.Stop());
	BOOST_CHECK(!controller.ProfilingStatus().running);

	BOOST_CHECK(controller.Start());
	BOOST_CHECK(controller.ProfilingStatus().running);
}

BOOST_AUTO_TEST_SUITE_END()
