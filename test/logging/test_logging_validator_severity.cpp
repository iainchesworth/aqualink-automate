#include <string>
#include <vector>

#include <boost/any.hpp>
#include <boost/program_options.hpp>
#include <boost/test/unit_test.hpp>

#include "logging/logging_severity_levels.h"
#include "options/validators/severity_level_validator.h"

#include "support/unit_test_ostream_support.h"

BOOST_AUTO_TEST_SUITE(LoggingValidator_Severity);

BOOST_AUTO_TEST_CASE(ValidSeverityCaseInsensitive)
{
    using namespace AqualinkAutomate::Logging;

    // Test all severity levels in various cases
    const std::vector<std::string> severities = { "TRACE", "debug", "iNFo", "Notify", "warning", "ERROR", "fatal" };
    const std::vector<Severity> expected_severities = { Severity::Trace, Severity::Debug, Severity::Info, Severity::Notify, Severity::Warning, Severity::Error, Severity::Fatal };

    for (int i = 0; i < severities.size(); ++i) 
    {
        boost::any val;
        Severity target;
        std::vector<std::string> values { severities[i] };

        validate(val, values, &target, 0);

        BOOST_CHECK_EQUAL(boost::any_cast<Severity>(val), expected_severities[i]);
    }
}

BOOST_AUTO_TEST_CASE(InvalidSeverity)
{
    using namespace AqualinkAutomate::Logging;

    boost::any val;
    std::vector<std::string> values;
    Severity target;

    // Test invalid severity
    values.push_back("invalid_severity");
    BOOST_CHECK_THROW(validate(val, values, &target, 0), boost::program_options::validation_error);
}

BOOST_AUTO_TEST_SUITE_END();
