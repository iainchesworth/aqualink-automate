#include <string>

#include <boost/test/unit_test.hpp>

#include "utility/value_debouncer.h"

using namespace AqualinkAutomate::Utility;

// Note: ValueDebouncer requires threshold+1 submissions to commit a value.
// The first submission sets the future value (count=0), then threshold more
// submissions increment the counter until it reaches the threshold to commit.

BOOST_AUTO_TEST_SUITE(TestSuite_Utility_ValueDebouncer)

BOOST_AUTO_TEST_CASE(Test_DefaultConstruction_ReturnsDefaultValue)
{
	ValueDebouncer<int> debouncer;
	BOOST_CHECK_EQUAL(debouncer(), 0);
}

BOOST_AUTO_TEST_CASE(Test_DefaultConstruction_String_ReturnsEmpty)
{
	ValueDebouncer<std::string> debouncer;
	BOOST_CHECK_EQUAL(debouncer(), std::string{});
}

BOOST_AUTO_TEST_CASE(Test_ValueNotCommittedBeforeThreshold)
{
	ValueDebouncer<int> debouncer(5);

	// First call sets future value, then 4 more (total 5, need 6 for threshold=5)
	for (int i = 0; i < 5; ++i)
	{
		debouncer(42);
	}

	// Value should not have been committed yet (need one more)
	BOOST_CHECK_EQUAL(debouncer(), 0);
}

BOOST_AUTO_TEST_CASE(Test_ValueCommittedAtThreshold)
{
	ValueDebouncer<int> debouncer(5);

	// Need threshold+1 submissions: 1 to set future + 5 to reach threshold
	for (int i = 0; i < 6; ++i)
	{
		debouncer(42);
	}

	BOOST_CHECK_EQUAL(debouncer(), 42);
}

BOOST_AUTO_TEST_CASE(Test_ThirdDistinctValueResetsCounter)
{
	ValueDebouncer<int> debouncer(3);

	// Start tracking value 10: first sets future, second increments
	debouncer(10);
	debouncer(10);

	// Before threshold reached, switch to a different value - resets
	debouncer(20);

	// Now 20 is the new future value - need threshold more
	for (int i = 0; i < 3; ++i)
	{
		debouncer(20);
	}

	BOOST_CHECK_EQUAL(debouncer(), 20);
}

BOOST_AUTO_TEST_CASE(Test_ThresholdOne_ImmediateCommit)
{
	ValueDebouncer<int> debouncer(1);

	// First call sets future, second reaches threshold
	debouncer(99);
	debouncer(99);
	BOOST_CHECK_EQUAL(debouncer(), 99);
}

BOOST_AUTO_TEST_CASE(Test_StringType)
{
	ValueDebouncer<std::string> debouncer(2);

	// Need 3 calls: 1 to set future + 2 to reach threshold
	debouncer(std::string("hello"));
	debouncer(std::string("hello"));
	debouncer(std::string("hello"));

	BOOST_CHECK_EQUAL(debouncer(), "hello");
}

BOOST_AUTO_TEST_CASE(Test_OperatorAssign_LValue)
{
	ValueDebouncer<int> debouncer(1);

	// First sets future, second commits
	debouncer = 77;
	debouncer = 77;
	BOOST_CHECK_EQUAL(debouncer(), 77);
}

BOOST_AUTO_TEST_CASE(Test_SameValueAsCurrentIsIgnored)
{
	ValueDebouncer<int> debouncer(1);

	// Commit value 50
	debouncer(50);
	debouncer(50);
	BOOST_CHECK_EQUAL(debouncer(), 50);

	// Submitting the same value as current should be a no-op
	debouncer(50);
	BOOST_CHECK_EQUAL(debouncer(), 50);
}

BOOST_AUTO_TEST_CASE(Test_OperatorAssign_RValue)
{
	ValueDebouncer<int> debouncer(1);

	int val1 = 88;
	debouncer = std::move(val1);
	int val2 = 88;
	debouncer = std::move(val2);
	BOOST_CHECK_EQUAL(debouncer(), 88);
}

BOOST_AUTO_TEST_CASE(Test_FutureValueResetOnNewDistinctValue)
{
	ValueDebouncer<int> debouncer(3);

	// Track value 10: first sets future, second increments count to 1
	debouncer(10);
	debouncer(10);

	// Switch to value 20 - resets counter, sets new future
	debouncer(20);

	// Go back to value 10 - resets counter again, sets new future
	debouncer(10);

	// Only submitted 10 once since last reset, should not commit
	BOOST_CHECK_EQUAL(debouncer(), 0);
}

BOOST_AUTO_TEST_CASE(Test_CustomComparator)
{
	// Case-insensitive string debouncer
	struct CaseInsensitiveEqual
	{
		bool operator()(const std::string& a, const std::string& b) const
		{
			if (a.size() != b.size()) return false;
			for (size_t i = 0; i < a.size(); ++i)
			{
				if (std::tolower(a[i]) != std::tolower(b[i])) return false;
			}
			return true;
		}
	};

	ValueDebouncer<std::string, CaseInsensitiveEqual> debouncer(2);

	// First sets future, then 2 more reach threshold
	debouncer(std::string("Hello"));
	debouncer(std::string("HELLO"));
	debouncer(std::string("hElLo"));

	// Should be committed because comparator treats them as equal
	BOOST_CHECK_EQUAL(debouncer(), "Hello");
}

BOOST_AUTO_TEST_CASE(Test_OperatorCall_LValue)
{
	ValueDebouncer<int> debouncer(1);
	const int val = 55;
	debouncer(val);
	debouncer(val);
	BOOST_CHECK_EQUAL(debouncer(), 55);
}

BOOST_AUTO_TEST_CASE(Test_OperatorCall_RValue)
{
	ValueDebouncer<int> debouncer(1);
	debouncer(int{66});
	debouncer(int{66});
	BOOST_CHECK_EQUAL(debouncer(), 66);
}

BOOST_AUTO_TEST_SUITE_END()
