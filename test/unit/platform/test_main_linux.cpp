#define BOOST_TEST_MODULE AqualinkAutomateTests

#include <boost/test/unit_test.hpp>

#include "logging/logging.h"
#include "logging/logging_initialise.h"
#include "logging/logging_severity_filter.h"

using namespace AqualinkAutomate;

//
// No specific main() under Linux due to the static library linkage for Boost + vcpkg.
//

struct UnitTestGlobalFixture
{
	UnitTestGlobalFixture()
	{
		// Turn off all logging barring FATAL messages to help not skew performance tests
		// by writting logging messages to the console.

		Logging::SeverityFiltering::SetGlobalFilterLevel(Logging::Severity::Fatal);
		Logging::Initialise();
	}

	void setup()
	{
	}

	void teardown()
	{
	}

	~UnitTestGlobalFixture()
	{
	}
};

BOOST_TEST_GLOBAL_FIXTURE(UnitTestGlobalFixture);
