#include <string>
#include <vector>

#include <boost/any.hpp>
#include <boost/program_options.hpp>
#include <boost/test/unit_test.hpp>

#include "profiling/types/profiling_types.h"
#include "options/validators/profiler_type_validator.h"

#include "support/unit_test_ostream_support.h"

BOOST_AUTO_TEST_SUITE(LoggingValidator_Profiler);

BOOST_AUTO_TEST_CASE(ValidProfilerCaseInsensitive)
{
    using namespace AqualinkAutomate::Types;

    // Test all profiler types in various cases
    const std::vector<std::string> profiler_types = { "tracy", "UPROF", "vTuNe" };
    const std::vector<ProfilerTypes> expected_profiler_types = { ProfilerTypes::Tracy, ProfilerTypes::UProf, ProfilerTypes::VTune };

    for (int i = 0; i < profiler_types.size(); ++i) 
    {
        boost::any val;
        ProfilerTypes target;
        std::vector<std::string> values { profiler_types[i] };

        validate(val, values, &target, 0);
        
        BOOST_CHECK_EQUAL(boost::any_cast<ProfilerTypes>(val), expected_profiler_types[i]);
    }
}

BOOST_AUTO_TEST_CASE(InvalidProfiler)
{
    using namespace AqualinkAutomate::Types;

    boost::any val;
    std::vector<std::string> values;
    ProfilerTypes target;

    // Test invalid profiler
    values.push_back("invalid_profiler");
    BOOST_CHECK_THROW(validate(val, values, &target, 0), boost::program_options::validation_error);
}

BOOST_AUTO_TEST_SUITE_END();
