#include <string>
#include <vector>

#include <boost/any.hpp>
#include <boost/program_options.hpp>
#include <boost/test/unit_test.hpp>

#include "logging/logging_severity_levels.h"
#include "options/validators/profiler_type_validator.h"
#include "options/validators/severity_level_validator.h"
#include "profiling/types/profiling_types.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate;

BOOST_AUTO_TEST_SUITE(TestSuite_OptionsValidators)

//=============================================================================
// SEVERITY VALIDATOR — empty / whitespace value (regression for the
// out-of-bounds read on word[0] in the old capitalize() lambda).
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_SeverityValidator_EmptyString_ThrowsNotCrashes)
{
	using namespace AqualinkAutomate::Logging;

	boost::any val;
	Severity target{};
	const std::vector<std::string> values{ std::string{} };

	// An empty option value (e.g. `--loglevel-main ""` or a bare
	// `loglevel-main =` config line) must fail cleanly with validation_error,
	// NOT read word[0] out of bounds.
	BOOST_CHECK_THROW(validate(val, values, &target, 0), boost::program_options::validation_error);
}

BOOST_AUTO_TEST_CASE(Test_SeverityValidator_WhitespaceOnly_Throws)
{
	using namespace AqualinkAutomate::Logging;

	boost::any val;
	Severity target{};
	const std::vector<std::string> values{ "   " };

	BOOST_CHECK_THROW(validate(val, values, &target, 0), boost::program_options::validation_error);
}

BOOST_AUTO_TEST_CASE(Test_SeverityValidator_HighBitChar_DoesNotInvokeUB)
{
	using namespace AqualinkAutomate::Logging;

	boost::any val;
	Severity target{};
	// A byte with the high bit set would be a (negative) char fed straight to
	// std::tolower in the old code (UB). The shared helper casts to unsigned
	// char first; this must simply fail to match -> validation_error.
	const std::vector<std::string> values{ std::string(1, static_cast<char>(0x80)) };

	BOOST_CHECK_THROW(validate(val, values, &target, 0), boost::program_options::validation_error);
}

BOOST_AUTO_TEST_CASE(Test_SeverityValidator_StillAcceptsValidCaseInsensitive)
{
	using namespace AqualinkAutomate::Logging;

	boost::any val;
	Severity target{};
	const std::vector<std::string> values{ "wArNiNg" };

	validate(val, values, &target, 0);
	BOOST_CHECK_EQUAL(boost::any_cast<Severity>(val), Severity::Warning);
}

//=============================================================================
// PROFILER VALIDATOR — empty value (shares the same helper).
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_ProfilerValidator_EmptyString_Throws)
{
	using namespace AqualinkAutomate::Types;

	boost::any val;
	ProfilerTypes target{};
	const std::vector<std::string> values{ std::string{} };

	BOOST_CHECK_THROW(validate(val, values, &target, 0), boost::program_options::validation_error);
}

BOOST_AUTO_TEST_CASE(Test_ProfilerValidator_StillAcceptsValidCaseInsensitive)
{
	using namespace AqualinkAutomate::Types;

	boost::any val;
	ProfilerTypes target{};
	const std::vector<std::string> values{ "tRaCy" };

	validate(val, values, &target, 0);
	BOOST_CHECK_EQUAL(boost::any_cast<ProfilerTypes>(val), ProfilerTypes::Tracy);
}

BOOST_AUTO_TEST_SUITE_END()
