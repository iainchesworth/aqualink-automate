#include <cstdint>
#include <optional>
#include <string_view>

#include <boost/test/unit_test.hpp>

#include "utility/to_number.h"

using namespace AqualinkAutomate::Utility;

BOOST_AUTO_TEST_SUITE(TestSuite_Utility_ToNumber)

// --- Valid conversions ---

BOOST_AUTO_TEST_CASE(Test_ToNumber_ValidInt)
{
	auto result = ToNumber<int>("42");
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result.value(), 42);
}

BOOST_AUTO_TEST_CASE(Test_ToNumber_ValidZero)
{
	auto result = ToNumber<int>("0");
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result.value(), 0);
}

BOOST_AUTO_TEST_CASE(Test_ToNumber_ValidNegative)
{
	auto result = ToNumber<int>("-123");
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result.value(), -123);
}

BOOST_AUTO_TEST_CASE(Test_ToNumber_ValidUint8)
{
	auto result = ToNumber<uint8_t>("255");
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result.value(), 255);
}

BOOST_AUTO_TEST_CASE(Test_ToNumber_ValidUint8_Zero)
{
	auto result = ToNumber<uint8_t>("0");
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result.value(), 0);
}

BOOST_AUTO_TEST_CASE(Test_ToNumber_ValidInt16)
{
	auto result = ToNumber<int16_t>("-32768");
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result.value(), -32768);
}

BOOST_AUTO_TEST_CASE(Test_ToNumber_ValidInt64)
{
	auto result = ToNumber<int64_t>("9999999999");
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result.value(), 9999999999LL);
}

// --- Invalid inputs ---

BOOST_AUTO_TEST_CASE(Test_ToNumber_EmptyString)
{
	auto result = ToNumber<int>("");
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(Test_ToNumber_Whitespace)
{
	auto result = ToNumber<int>(" 42");
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(Test_ToNumber_NonNumeric)
{
	auto result = ToNumber<int>("abc");
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(Test_ToNumber_LeadingPlus)
{
	auto result = ToNumber<int>("+42");
	BOOST_CHECK(!result.has_value());
}

// --- Out of range ---

BOOST_AUTO_TEST_CASE(Test_ToNumber_Uint8_OutOfRange)
{
	auto result = ToNumber<uint8_t>("256");
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(Test_ToNumber_Int8_OutOfRange_Below)
{
	auto result = ToNumber<int8_t>("-129");
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(Test_ToNumber_Int8_OutOfRange_Above)
{
	auto result = ToNumber<int8_t>("128");
	BOOST_CHECK(!result.has_value());
}

// --- Edge cases ---

BOOST_AUTO_TEST_CASE(Test_ToNumber_LeadingZeros)
{
	auto result = ToNumber<int>("007");
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result.value(), 7);
}

BOOST_AUTO_TEST_CASE(Test_ToNumber_PartialParse_RejectsTrailingChars)
{
	// ToNumber verifies the entire string was consumed.
	auto result = ToNumber<int>("12abc");
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(Test_ToNumber_TrailingWhitespace_Rejected)
{
	auto result = ToNumber<int>("42 ");
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(Test_ToNumber_TrailingDot_Rejected)
{
	auto result = ToNumber<int>("42.");
	BOOST_CHECK(!result.has_value());
}

// --- Floating-point path (broadened Number concept now permits this) ---

BOOST_AUTO_TEST_CASE(Test_ToNumber_ValidDouble)
{
	auto result = ToNumber<double>("3.14");
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_CLOSE(result.value(), 3.14, 0.0001);
}

BOOST_AUTO_TEST_CASE(Test_ToNumber_ValidFloat_Negative)
{
	auto result = ToNumber<float>("-0.5");
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_CLOSE(result.value(), -0.5f, 0.0001f);
}

BOOST_AUTO_TEST_CASE(Test_ToNumber_ValidDouble_Integer)
{
	auto result = ToNumber<double>("100");
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_CLOSE(result.value(), 100.0, 0.0001);
}

BOOST_AUTO_TEST_CASE(Test_ToNumber_Double_NonNumeric_Rejected)
{
	auto result = ToNumber<double>("abc");
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(Test_ToNumber_Double_PartialParse_Rejected)
{
	auto result = ToNumber<double>("3.14xyz");
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END()
