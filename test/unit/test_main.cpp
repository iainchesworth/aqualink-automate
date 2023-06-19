#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_MODULE AqualinkAutomateTests

#include <boost/test/unit_test.hpp>

#include "logging/logging.h"
#include "logging/logging_initialise.h"
#include "logging/logging_severity_filter.h"

using namespace AqualinkAutomate;

int main(int argc, char* argv[]) 
{
    // Turn off all logging barring FATAL messages to help not skew performance tests
    // by writting logging messages to the console.

    Logging::SeverityFiltering::SetGlobalFilterLevel(Logging::Severity::Fatal);
    Logging::Initialise();

    return boost::unit_test::unit_test_main(&init_unit_test, argc, argv);
}
