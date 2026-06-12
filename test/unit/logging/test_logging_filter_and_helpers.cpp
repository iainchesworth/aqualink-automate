#include <string_view>

#include <boost/test/unit_test.hpp>

#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_utility.hpp>

#include "logging/logging.h"
#include "logging/logging_channels.h"
#include "logging/logging_severity_filter.h"
#include "logging/logging_severity_levels.h"

#include "support/unit_test_ostream_support.h"

//
// Regression coverage for the array-backed per-channel severity filter and the
// constexpr source-file basename helper introduced in WU-OBS-LOGGING.
//
// The filter was migrated from a std::map<Channel, Severity> (per-call red-black-tree
// lookup that "failed open" for any missing channel) to a std::array indexed by the
// Channel enum value. These tests assert the array is exhaustive (every enumerator is
// addressable), the read path is non-mutating, and ShouldLog agrees with the configured
// level for every channel - i.e. there is no fail-open path any more.
//

BOOST_AUTO_TEST_SUITE(LoggingFilterAndHelpers);

BOOST_AUTO_TEST_CASE(ShouldLogMatchesConfiguredLevelForEveryChannel)
{
	using namespace AqualinkAutomate::Logging;
	using namespace AqualinkAutomate::Logging::SeverityFiltering;

	// Drive the whole table to a non-default, mid-range level so we exercise both the
	// "below" and "at/above" branches of ShouldLog for every enumerator.
	SetGlobalFilterLevel(Severity::Warning);

	magic_enum::enum_for_each<Channel>([](auto const& channel)
		{
			const Channel ch = channel.value;

			// At/above the configured level must log; below must not. Critically, NO
			// channel may "fail open" by reporting Trace as its level.
			BOOST_CHECK_EQUAL(GetChannelFilterLevel(ch), Severity::Warning);
			BOOST_CHECK(ShouldLog(ch, Severity::Warning));
			BOOST_CHECK(ShouldLog(ch, Severity::Error));
			BOOST_CHECK(ShouldLog(ch, Severity::Fatal));
			BOOST_CHECK(!ShouldLog(ch, Severity::Info));
			BOOST_CHECK(!ShouldLog(ch, Severity::Debug));
			BOOST_CHECK(!ShouldLog(ch, Severity::Trace));
		}
	);

	// Restore the documented baseline so test ordering cannot leak this state.
	SetGlobalFilterLevel(DEFAULT_SEVERITY);
}

BOOST_AUTO_TEST_CASE(PerChannelSettingsAreIndependent)
{
	using namespace AqualinkAutomate::Logging;
	using namespace AqualinkAutomate::Logging::SeverityFiltering;

	SetGlobalFilterLevel(DEFAULT_SEVERITY);

	// Setting one channel must not perturb a sibling - the array indices are distinct.
	SetChannelFilterLevel(Channel::Messages, Severity::Trace);
	SetChannelFilterLevel(Channel::Protocol, Severity::Error);

	BOOST_CHECK_EQUAL(GetChannelFilterLevel(Channel::Messages), Severity::Trace);
	BOOST_CHECK_EQUAL(GetChannelFilterLevel(Channel::Protocol), Severity::Error);
	BOOST_CHECK_EQUAL(GetChannelFilterLevel(Channel::Serial), DEFAULT_SEVERITY);

	// Reads are non-mutating: repeated reads return the same value.
	BOOST_CHECK_EQUAL(GetChannelFilterLevel(Channel::Messages), Severity::Trace);
	BOOST_CHECK_EQUAL(GetChannelFilterLevel(Channel::Protocol), Severity::Error);

	SetGlobalFilterLevel(DEFAULT_SEVERITY);
}

BOOST_AUTO_TEST_CASE(ChannelCountMatchesEnumCount)
{
	using namespace AqualinkAutomate::Logging;

	// The static_assert in logging.h pins this at compile time; assert it at runtime too
	// so a drift between the enum and the wired-up logger set is caught loudly.
	BOOST_CHECK_EQUAL(CHANNEL_COUNT, magic_enum::enum_count<Channel>());
}

BOOST_AUTO_TEST_CASE(SourceFileBasenameStripsLeadingPath)
{
	using AqualinkAutomate::Logging::SourceFileBasename;

	// POSIX separator.
	BOOST_CHECK(SourceFileBasename("/abs/path/to/source.cpp") == std::string_view("source.cpp"));

	// Windows separator (as produced by MSVC __FILE__ / std::source_location).
	BOOST_CHECK(SourceFileBasename("R:\\repo\\src\\core\\logging\\logging.h") == std::string_view("logging.h"));

	// Mixed separators - last separator of either kind wins.
	BOOST_CHECK(SourceFileBasename("C:/repo\\src/file.cpp") == std::string_view("file.cpp"));

	// No separator: the whole string is the basename.
	BOOST_CHECK(SourceFileBasename("file.cpp") == std::string_view("file.cpp"));

	// Empty string is returned unchanged (no out-of-bounds scan).
	BOOST_CHECK(SourceFileBasename("") == std::string_view(""));
}

BOOST_AUTO_TEST_CASE(SourceFileBasenameIsConstexpr)
{
	using AqualinkAutomate::Logging::SourceFileBasename;

	// Force compile-time evaluation to prove the helper is genuinely constexpr (and thus
	// imposes no per-record runtime cost for literal paths).
	static constexpr std::string_view basename = SourceFileBasename("/a/b/c.cpp");
	static_assert(basename == std::string_view("c.cpp"), "SourceFileBasename must be constexpr-evaluable");

	BOOST_CHECK(basename == std::string_view("c.cpp"));
}

BOOST_AUTO_TEST_SUITE_END();
