#include <string>

#include <boost/test/unit_test.hpp>

#include "utility/string_manipulation.h"

using namespace AqualinkAutomate::Utility;

BOOST_AUTO_TEST_SUITE(StringManipulation_TestSuite)

// =============================================================================
// TrimWhitespace
// =============================================================================

BOOST_AUTO_TEST_CASE(TestTrim_NoWhitespace)
{
	BOOST_CHECK_EQUAL(TrimWhitespace("hello"), "hello");
}

BOOST_AUTO_TEST_CASE(TestTrim_LeadingSpaces)
{
	BOOST_CHECK_EQUAL(TrimWhitespace("   hello"), "hello");
}

BOOST_AUTO_TEST_CASE(TestTrim_TrailingSpaces)
{
	BOOST_CHECK_EQUAL(TrimWhitespace("hello   "), "hello");
}

BOOST_AUTO_TEST_CASE(TestTrim_BothSides)
{
	BOOST_CHECK_EQUAL(TrimWhitespace("  hello  "), "hello");
}

BOOST_AUTO_TEST_CASE(TestTrim_InternalSpacesPreserved)
{
	BOOST_CHECK_EQUAL(TrimWhitespace("  hello world  "), "hello world");
}

BOOST_AUTO_TEST_CASE(TestTrim_Tabs)
{
	BOOST_CHECK_EQUAL(TrimWhitespace("\thello\t"), "hello");
}

BOOST_AUTO_TEST_CASE(TestTrim_Newlines)
{
	BOOST_CHECK_EQUAL(TrimWhitespace("\nhello\n"), "hello");
}

BOOST_AUTO_TEST_CASE(TestTrim_MixedWhitespace)
{
	BOOST_CHECK_EQUAL(TrimWhitespace(" \t\nhello \t\n"), "hello");
}

BOOST_AUTO_TEST_CASE(TestTrim_EmptyString)
{
	BOOST_CHECK_EQUAL(TrimWhitespace(""), "");
}

BOOST_AUTO_TEST_CASE(TestTrim_AllWhitespace)
{
	BOOST_CHECK_EQUAL(TrimWhitespace("   "), "");
}

BOOST_AUTO_TEST_CASE(TestTrim_SingleChar)
{
	BOOST_CHECK_EQUAL(TrimWhitespace("x"), "x");
}

BOOST_AUTO_TEST_CASE(TestTrim_SingleSpace)
{
	BOOST_CHECK_EQUAL(TrimWhitespace(" "), "");
}

BOOST_AUTO_TEST_SUITE_END()
