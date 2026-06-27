#include <optional>
#include <source_location>

#include <boost/test/unit_test.hpp>

#include "profiling/factories/profiler_factory.h"
#include "profiling/factories/profiling_unit_factory.h"
#include "profiling/types/profiling_types.h"

using namespace AqualinkAutomate;

BOOST_AUTO_TEST_SUITE(ProfilingFactories_TestSuite)

// =============================================================================
// ProfilerFactory::Get() returns a stable instance (no per-call allocation).
//
// Regression guard for WU-OBS-PROFILING: the NoOp fallback was previously
// heap-allocated (make_shared) on every Get() call on the per-frame hot path.
// It is now cached, so consecutive calls must yield the same pointer. This
// holds whether the NoOp fallback or a registered profiler is returned, so the
// assertion is robust to singleton state set by other test cases.
// =============================================================================

BOOST_AUTO_TEST_CASE(ProfilerFactory_Get_ReturnsNonNull)
{
	auto profiler = Factory::ProfilerFactory::Instance().Get();
	BOOST_REQUIRE(nullptr != profiler);
}

BOOST_AUTO_TEST_CASE(ProfilerFactory_Get_ReturnsStableInstanceAcrossCalls)
{
	auto first = Factory::ProfilerFactory::Instance().Get();
	auto second = Factory::ProfilerFactory::Instance().Get();

	BOOST_REQUIRE(nullptr != first);
	BOOST_REQUIRE(nullptr != second);
	BOOST_CHECK_EQUAL(first.get(), second.get());
}

// =============================================================================
// ProfilingUnitFactory create methods produce valid, safe units.
//
// On the no-op path the returned unit must be non-null and its annotation /
// lifecycle methods must be safe no-ops (the const-reference generator binding
// must still invoke the generator correctly).
// =============================================================================

BOOST_AUTO_TEST_CASE(ProfilingUnitFactory_CreateZone_ReturnsUsableUnit)
{
	auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("ProfilingFactories_TestSuite::CreateZone", std::source_location::current());

	BOOST_REQUIRE(nullptr != zone);
	BOOST_CHECK_NO_THROW(zone->Start());
	BOOST_CHECK_NO_THROW(zone->Mark());
	BOOST_CHECK_NO_THROW(zone->Text("annotation"));
	BOOST_CHECK_NO_THROW(zone->Value(42));
	BOOST_CHECK_NO_THROW(zone->End());
}

BOOST_AUTO_TEST_CASE(ProfilingUnitFactory_CreateFrame_ReturnsUsableUnit)
{
	auto frame = Factory::ProfilingUnitFactory::Instance().CreateFrame("ProfilingFactories_TestSuite::CreateFrame");

	BOOST_REQUIRE(nullptr != frame);
	BOOST_CHECK_NO_THROW(frame->Start());
	BOOST_CHECK_NO_THROW(frame->Mark());
	BOOST_CHECK_NO_THROW(frame->End());
}

BOOST_AUTO_TEST_CASE(ProfilingUnitFactory_CreateDomain_ReturnsUsableUnit)
{
	auto domain = Factory::ProfilingUnitFactory::Instance().CreateDomain("ProfilingFactories_TestSuite::CreateDomain");

	BOOST_REQUIRE(nullptr != domain);
	BOOST_CHECK_NO_THROW(domain->Start());
	BOOST_CHECK_NO_THROW(domain->Mark());
	BOOST_CHECK_NO_THROW(domain->End());
}

BOOST_AUTO_TEST_CASE(ProfilingUnitFactory_CreateZone_DistinctCallsAreIndependentUnits)
{
	auto first = Factory::ProfilingUnitFactory::Instance().CreateZone("ProfilingFactories_TestSuite::Independent_A", std::source_location::current());
	auto second = Factory::ProfilingUnitFactory::Instance().CreateZone("ProfilingFactories_TestSuite::Independent_B", std::source_location::current());

	BOOST_REQUIRE(nullptr != first);
	BOOST_REQUIRE(nullptr != second);
	BOOST_CHECK_NE(first.get(), second.get());
}

// =============================================================================
// Introspection accessors used by the runtime control surface / startup warning.
//
// RegisteredTypes()/IsRegistered() must agree, and selecting a type that was NOT
// registered must leave Get() returning a usable (NoOp) profiler rather than null
// or throwing — the safety net behind the "requested profiler not available in
// this build" warning.
// =============================================================================

BOOST_AUTO_TEST_CASE(ProfilerFactory_RegisteredTypes_AgreeWithIsRegistered)
{
	auto& factory = Factory::ProfilerFactory::Instance();

	for (const auto type : factory.RegisteredTypes())
	{
		BOOST_CHECK(factory.IsRegistered(type));
	}
}

BOOST_AUTO_TEST_CASE(ProfilerFactory_SelectingUnregisteredType_FallsBackToUsableProfiler)
{
	auto& factory = Factory::ProfilerFactory::Instance();

	// Pick a type that is NOT registered in this (no-RegisterAvailableProfilers)
	// test process; if somehow all are registered, this test is a no-op pass.
	std::optional<Types::ProfilerTypes> unregistered;
	for (const auto candidate : { Types::ProfilerTypes::Tracy, Types::ProfilerTypes::UProf, Types::ProfilerTypes::VTune })
	{
		if (!factory.IsRegistered(candidate))
		{
			unregistered = candidate;
			break;
		}
	}

	if (unregistered.has_value())
	{
		factory.SetDesired(*unregistered);
		BOOST_CHECK(factory.Selected().has_value());

		// Desired-but-unregistered must still yield a usable profiler (NoOp), never null.
		auto profiler = factory.Get();
		BOOST_REQUIRE(nullptr != profiler);
		BOOST_CHECK_NO_THROW(profiler->StartProfiling());
		BOOST_CHECK_NO_THROW(profiler->Resume());
		BOOST_CHECK_NO_THROW(profiler->Pause());
		BOOST_CHECK_NO_THROW(profiler->EmitFrameMark("test"));
	}
}

BOOST_AUTO_TEST_SUITE_END()
