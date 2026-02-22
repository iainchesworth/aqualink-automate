#include <format>
#include <sstream>
#include <string>

#include <boost/test/unit_test.hpp>

#include "jandy/formatters/chemistry_formatter.h"
#include "jandy/utility/string_conversion/chemistry_string_converter.h"

using namespace AqualinkAutomate::Utility;

BOOST_AUTO_TEST_SUITE(Formatters_ChemistryTestSuite)

// =============================================================================
// Valid ChemistryStringConverter
// =============================================================================

BOOST_AUTO_TEST_CASE(TestFormat_ValidChemistry_ContainsORPAndPH)
{
	ChemistryStringConverter chemistry;
	chemistry = std::string("ORP/650 PH/7.4");

	auto result = std::format("{}", chemistry);

	BOOST_CHECK(result.find("ORP=") != std::string::npos);
	BOOST_CHECK(result.find("PH=") != std::string::npos);
}

// =============================================================================
// Invalid/Error State
// =============================================================================

BOOST_AUTO_TEST_CASE(TestFormat_DefaultChemistry_ShowsError)
{
	ChemistryStringConverter chemistry;

	auto result = std::format("{}", chemistry);

	// Bug 7 fix: default-constructed converter is now in error state
	BOOST_CHECK_EQUAL(result, "CHEM=??");
}

BOOST_AUTO_TEST_CASE(TestFormat_InvalidString_ShowsError)
{
	ChemistryStringConverter chemistry;
	chemistry = std::string("INVALID");

	auto result = std::format("{}", chemistry);

	BOOST_CHECK_EQUAL(result, "CHEM=??");
}

// =============================================================================
// ostream operator
// =============================================================================

BOOST_AUTO_TEST_CASE(TestOstream_Chemistry_MatchesFormat)
{
	ChemistryStringConverter chemistry;

	std::ostringstream oss;
	oss << chemistry;

	BOOST_CHECK_EQUAL(oss.str(), std::format("{}", chemistry));
}

BOOST_AUTO_TEST_CASE(TestOstream_ValidChemistry_MatchesFormat)
{
	ChemistryStringConverter chemistry;
	chemistry = std::string("ORP/650 PH/7.4");

	std::ostringstream oss;
	oss << chemistry;

	BOOST_CHECK_EQUAL(oss.str(), std::format("{}", chemistry));
}

BOOST_AUTO_TEST_SUITE_END()
