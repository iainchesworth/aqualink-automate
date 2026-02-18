#define BOOST_TEST_MODULE AqualinkAutomateIntegrationTests

#include <boost/test/unit_test.hpp>

#include "logging/logging.h"
#include "logging/logging_initialise.h"
#include "logging/logging_severity_filter.h"

#include "profiling/profiling.h"

using namespace AqualinkAutomate;

struct IntegrationTestGlobalFixture
{
	IntegrationTestGlobalFixture()
	{
		Logging::SeverityFiltering::SetGlobalFilterLevel(Logging::Severity::Fatal);
		Logging::Initialise();
	}

	void setup()
	{
		if (auto profiler = Factory::ProfilerFactory::Instance().Get(); nullptr != profiler)
		{
			profiler->StartProfiling();
		}
	}

	void teardown()
	{
		if (auto profiler = Factory::ProfilerFactory::Instance().Get(); nullptr != profiler)
		{
			profiler->StopProfiling();
		}
	}

	~IntegrationTestGlobalFixture() = default;
};

BOOST_TEST_GLOBAL_FIXTURE(IntegrationTestGlobalFixture);
