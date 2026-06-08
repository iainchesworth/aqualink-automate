#include <boost/test/unit_test.hpp>

#include "utility/case_insensitive_comparision.h"

using namespace AqualinkAutomate::Utility;

BOOST_AUTO_TEST_SUITE(TestSuite_Utility_CaseInsensitiveComparison)

// --- Canonical PascalCase helper ---

BOOST_AUTO_TEST_CASE(Test_SameLetter_DifferentCase_AreEqual)
{
	BOOST_CHECK(CaseInsensitiveComparison('a', 'A'));
	BOOST_CHECK(CaseInsensitiveComparison('Z', 'z'));
}

BOOST_AUTO_TEST_CASE(Test_SameLetter_SameCase_AreEqual)
{
	BOOST_CHECK(CaseInsensitiveComparison('m', 'm'));
	BOOST_CHECK(CaseInsensitiveComparison('M', 'M'));
}

BOOST_AUTO_TEST_CASE(Test_DifferentLetters_AreNotEqual)
{
	BOOST_CHECK(!CaseInsensitiveComparison('a', 'b'));
	BOOST_CHECK(!CaseInsensitiveComparison('A', 'b'));
}

BOOST_AUTO_TEST_CASE(Test_Digits_CompareByValue)
{
	BOOST_CHECK(CaseInsensitiveComparison('7', '7'));
	BOOST_CHECK(!CaseInsensitiveComparison('7', '8'));
}

// --- Legacy mis-spelled alias still forwards (cross-unit source compatibility) ---

BOOST_AUTO_TEST_CASE(Test_LegacyAlias_ForwardsToCanonical)
{
	BOOST_CHECK(case_insensitive_comparision('a', 'A'));
	BOOST_CHECK(!case_insensitive_comparision('a', 'b'));
}

BOOST_AUTO_TEST_SUITE_END()
