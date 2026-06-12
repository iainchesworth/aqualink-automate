#include <boost/test/unit_test.hpp>

#include "logging/logging_channels.h"
#include "logging/logging_severity_filter.h"
#include "logging/logging_severity_levels.h"

#include "support/unit_test_ostream_support.h"

//
// Regression coverage for the per-channel severity-defaults map.
//
// Every Channel enumerator must resolve to the configured DEFAULT_SEVERITY rather
// than falling through a mutating operator[] lookup to the most-verbose level
// (Severity::Trace). Channel::Coroutines and Channel::Developer were previously
// absent from MinimumSeverityLevelPerChannel, so reads for those channels silently
// inserted a default-constructed Severity (Trace) instead of DEFAULT_SEVERITY.
//

BOOST_AUTO_TEST_SUITE(LoggingSeverityFilter_Defaults);

BOOST_AUTO_TEST_CASE(EveryChannelDefaultsToDefaultSeverity)
{
    using namespace AqualinkAutomate::Logging;
    using namespace AqualinkAutomate::Logging::SeverityFiltering;

    // Establish the documented baseline: the filter level the application applies
    // for every channel at start-up.
    SetGlobalFilterLevel(DEFAULT_SEVERITY);

    // Each channel must report DEFAULT_SEVERITY - and emphatically NOT the most
    // verbose Severity::Trace, which is what a missing map entry would yield.
    BOOST_CHECK_EQUAL(GetChannelFilterLevel(Channel::Developer), DEFAULT_SEVERITY);
    BOOST_CHECK_EQUAL(GetChannelFilterLevel(Channel::Coroutines), DEFAULT_SEVERITY);

    BOOST_CHECK_NE(GetChannelFilterLevel(Channel::Developer), Severity::Trace);
    BOOST_CHECK_NE(GetChannelFilterLevel(Channel::Coroutines), Severity::Trace);
}

BOOST_AUTO_TEST_CASE(ReadPathDoesNotResetConfiguredChannels)
{
    using namespace AqualinkAutomate::Logging;
    using namespace AqualinkAutomate::Logging::SeverityFiltering;

    // Configure the two previously-missing channels to a non-default value...
    SetChannelFilterLevel(Channel::Developer, Severity::Error);
    SetChannelFilterLevel(Channel::Coroutines, Severity::Error);

    // ...and confirm reads return the configured value (i.e. the channels are
    // genuine map entries and the read path is non-mutating).
    BOOST_CHECK_EQUAL(GetChannelFilterLevel(Channel::Developer), Severity::Error);
    BOOST_CHECK_EQUAL(GetChannelFilterLevel(Channel::Coroutines), Severity::Error);

    // A repeated read must not perturb the stored value.
    BOOST_CHECK_EQUAL(GetChannelFilterLevel(Channel::Developer), Severity::Error);
    BOOST_CHECK_EQUAL(GetChannelFilterLevel(Channel::Coroutines), Severity::Error);

    // Restore the default baseline so test ordering cannot leak this state.
    SetChannelFilterLevel(Channel::Developer, DEFAULT_SEVERITY);
    SetChannelFilterLevel(Channel::Coroutines, DEFAULT_SEVERITY);
}

BOOST_AUTO_TEST_SUITE_END();
