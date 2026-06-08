#include <chrono>
#include <string>

#include <boost/test/unit_test.hpp>

#include "utility/timeout_duration_string_converter.h"

using namespace AqualinkAutomate::Utility;
using namespace std::chrono_literals;

BOOST_AUTO_TEST_SUITE(TestSuite_Utility_TimeoutDurationStringConverter)

// --- Valid conversions ---

BOOST_AUTO_TEST_CASE(Test_ValidZero)
{
	TimeoutDurationStringConverter converter("00:00:00");
	BOOST_CHECK_EQUAL(converter().count(), 0);
}

BOOST_AUTO_TEST_CASE(Test_ValidHoursMinutesSeconds)
{
	TimeoutDurationStringConverter converter("01:30:45");
	auto expected = std::chrono::hours(1) + std::chrono::minutes(30) + std::chrono::seconds(45);
	BOOST_CHECK_EQUAL(converter().count(), std::chrono::duration_cast<std::chrono::seconds>(expected).count());
}

BOOST_AUTO_TEST_CASE(Test_ValidMaxValues)
{
	TimeoutDurationStringConverter converter("99:59:59");
	auto expected = std::chrono::hours(99) + std::chrono::minutes(59) + std::chrono::seconds(59);
	BOOST_CHECK_EQUAL(converter().count(), std::chrono::duration_cast<std::chrono::seconds>(expected).count());
}

BOOST_AUTO_TEST_CASE(Test_ValidOnlyHours)
{
	TimeoutDurationStringConverter converter("05:00:00");
	BOOST_CHECK_EQUAL(converter().count(), std::chrono::duration_cast<std::chrono::seconds>(std::chrono::hours(5)).count());
}

BOOST_AUTO_TEST_CASE(Test_ValidOnlyMinutes)
{
	TimeoutDurationStringConverter converter("00:45:00");
	BOOST_CHECK_EQUAL(converter().count(), std::chrono::duration_cast<std::chrono::seconds>(std::chrono::minutes(45)).count());
}

BOOST_AUTO_TEST_CASE(Test_ValidOnlySeconds)
{
	TimeoutDurationStringConverter converter("00:00:30");
	BOOST_CHECK_EQUAL(converter().count(), 30);
}

// --- Invalid length ---

BOOST_AUTO_TEST_CASE(Test_InvalidLength_TooShort)
{
	TimeoutDurationStringConverter converter("01:30");
	BOOST_CHECK_EQUAL(converter().count(), 0);
}

BOOST_AUTO_TEST_CASE(Test_InvalidLength_TooLong)
{
	TimeoutDurationStringConverter converter("001:30:45");
	BOOST_CHECK_EQUAL(converter().count(), 0);
}

BOOST_AUTO_TEST_CASE(Test_InvalidLength_Empty)
{
	TimeoutDurationStringConverter converter("");
	BOOST_CHECK_EQUAL(converter().count(), 0);
}

// --- Invalid delimiters ---

BOOST_AUTO_TEST_CASE(Test_InvalidDelimiter_First)
{
	TimeoutDurationStringConverter converter("01-30:45");
	BOOST_CHECK_EQUAL(converter().count(), 0);
}

BOOST_AUTO_TEST_CASE(Test_InvalidDelimiter_Second)
{
	TimeoutDurationStringConverter converter("01:30-45");
	BOOST_CHECK_EQUAL(converter().count(), 0);
}

// --- Invalid numbers ---

BOOST_AUTO_TEST_CASE(Test_InvalidHours_NonNumeric)
{
	TimeoutDurationStringConverter converter("AB:30:45");
	BOOST_CHECK_EQUAL(converter().count(), 0);
}

BOOST_AUTO_TEST_CASE(Test_InvalidMinutes_NonNumeric)
{
	TimeoutDurationStringConverter converter("01:XY:45");
	BOOST_CHECK_EQUAL(converter().count(), 0);
}

BOOST_AUTO_TEST_CASE(Test_InvalidSeconds_NonNumeric)
{
	TimeoutDurationStringConverter converter("01:30:ZZ");
	BOOST_CHECK_EQUAL(converter().count(), 0);
}

// --- Out of range ---

BOOST_AUTO_TEST_CASE(Test_OutOfRange_Minutes60)
{
	TimeoutDurationStringConverter converter("01:60:00");
	BOOST_CHECK_EQUAL(converter().count(), 0);
}

BOOST_AUTO_TEST_CASE(Test_OutOfRange_Seconds60)
{
	TimeoutDurationStringConverter converter("01:00:60");
	BOOST_CHECK_EQUAL(converter().count(), 0);
}

// --- Default constructor ---

BOOST_AUTO_TEST_CASE(Test_DefaultConstructor)
{
	TimeoutDurationStringConverter converter;
	BOOST_CHECK_EQUAL(converter().count(), 0);
}

// --- Copy/Move constructors ---

BOOST_AUTO_TEST_CASE(Test_CopyConstructor)
{
	TimeoutDurationStringConverter original("01:30:45");
	TimeoutDurationStringConverter copy(original);
	BOOST_CHECK_EQUAL(copy().count(), original().count());
}

BOOST_AUTO_TEST_CASE(Test_MoveConstructor)
{
	TimeoutDurationStringConverter original("02:15:30");
	auto expected = original().count();
	TimeoutDurationStringConverter moved(std::move(original));
	BOOST_CHECK_EQUAL(moved().count(), expected);
}

// --- Copy/Move assignment ---

BOOST_AUTO_TEST_CASE(Test_CopyAssignment)
{
	TimeoutDurationStringConverter original("01:30:45");
	TimeoutDurationStringConverter other;
	other = original;
	BOOST_CHECK_EQUAL(other().count(), original().count());
}

BOOST_AUTO_TEST_CASE(Test_MoveAssignment)
{
	TimeoutDurationStringConverter original("02:15:30");
	auto expected = original().count();
	TimeoutDurationStringConverter other;
	other = std::move(original);
	BOOST_CHECK_EQUAL(other().count(), expected);
}

// --- Assignment from string ---

BOOST_AUTO_TEST_CASE(Test_AssignFromString)
{
	TimeoutDurationStringConverter converter;
	converter = std::string("03:45:15");
	auto expected = std::chrono::hours(3) + std::chrono::minutes(45) + std::chrono::seconds(15);
	BOOST_CHECK_EQUAL(converter().count(), std::chrono::duration_cast<std::chrono::seconds>(expected).count());
}

BOOST_AUTO_TEST_CASE(Test_ReassignFromString_Overwrites)
{
	TimeoutDurationStringConverter converter("01:00:00");
	BOOST_CHECK_EQUAL(converter().count(), 3600);

	converter = std::string("00:30:00");
	BOOST_CHECK_EQUAL(converter().count(), 1800);
}

// --- Regression: an invalid reassignment must NOT retain the previous duration ---

BOOST_AUTO_TEST_CASE(Test_ReassignInvalid_ResetsToZero_BadLength)
{
	// Previously, a parse failure left m_TimeoutDuration unchanged, so a valid value
	// followed by an invalid string silently kept reporting the old (stale) duration.
	TimeoutDurationStringConverter converter("01:00:00");
	BOOST_CHECK_EQUAL(converter().count(), 3600);

	converter = std::string("garbage");
	BOOST_CHECK_EQUAL(converter().count(), 0);
}

BOOST_AUTO_TEST_CASE(Test_ReassignInvalid_ResetsToZero_BadDelimiter)
{
	TimeoutDurationStringConverter converter("02:15:30");
	BOOST_CHECK(converter().count() > 0);

	converter = std::string("02-15-30");
	BOOST_CHECK_EQUAL(converter().count(), 0);
}

BOOST_AUTO_TEST_CASE(Test_ReassignInvalid_ResetsToZero_OutOfRange)
{
	TimeoutDurationStringConverter converter("01:30:45");
	BOOST_CHECK(converter().count() > 0);

	converter = std::string("01:60:00");
	BOOST_CHECK_EQUAL(converter().count(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
