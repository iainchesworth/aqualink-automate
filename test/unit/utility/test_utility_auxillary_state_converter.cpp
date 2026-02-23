#include <string>

#include <boost/test/unit_test.hpp>

#include "kernel/auxillary_devices/auxillary_status.h"
#include "utility/string_conversion/auxillary_state_string_converter.h"

using namespace AqualinkAutomate::Kernel;
using namespace AqualinkAutomate::Utility;

BOOST_AUTO_TEST_SUITE(AuxillaryStateStringConverter_TestSuite)

// =============================================================================
// Default construction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDefault_LabelReturnsEmpty)
{
	AuxillaryStateStringConverter converter;
	BOOST_CHECK(!converter.Label().has_value());
}

BOOST_AUTO_TEST_CASE(TestDefault_StateReturnsUnknown)
{
	AuxillaryStateStringConverter converter;
	BOOST_CHECK(!converter.State().has_value());
}

// =============================================================================
// Valid conversions
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConvert_OnState)
{
	AuxillaryStateStringConverter converter("Filter Pump  ON");
	BOOST_REQUIRE(converter.Label().has_value());
	BOOST_REQUIRE(converter.State().has_value());
	BOOST_CHECK_EQUAL(converter.Label().value(), "Filter Pump");
	BOOST_CHECK(converter.State().value() == AuxillaryStatuses::On);
}

BOOST_AUTO_TEST_CASE(TestConvert_OffState)
{
	AuxillaryStateStringConverter converter("Spa Mode    OFF");
	BOOST_REQUIRE(converter.Label().has_value());
	BOOST_REQUIRE(converter.State().has_value());
	BOOST_CHECK_EQUAL(converter.Label().value(), "Spa Mode");
	BOOST_CHECK(converter.State().value() == AuxillaryStatuses::Off);
}

BOOST_AUTO_TEST_CASE(TestConvert_EnabledState)
{
	AuxillaryStateStringConverter converter("Pool Heat   ENA");
	BOOST_REQUIRE(converter.Label().has_value());
	BOOST_REQUIRE(converter.State().has_value());
	BOOST_CHECK_EQUAL(converter.Label().value(), "Pool Heat");
	BOOST_CHECK(converter.State().value() == AuxillaryStatuses::Enabled);
}

BOOST_AUTO_TEST_CASE(TestConvert_PendingState)
{
	AuxillaryStateStringConverter converter("Spa Heat    ***");
	BOOST_REQUIRE(converter.Label().has_value());
	BOOST_REQUIRE(converter.State().has_value());
	BOOST_CHECK_EQUAL(converter.Label().value(), "Spa Heat");
	BOOST_CHECK(converter.State().value() == AuxillaryStatuses::Pending);
}

// =============================================================================
// Invalid inputs
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConvert_TooShort_ReturnsError)
{
	AuxillaryStateStringConverter converter("AB");
	BOOST_CHECK(!converter.Label().has_value());
	BOOST_CHECK(!converter.State().has_value());
}

BOOST_AUTO_TEST_CASE(TestConvert_TooLong_ReturnsError)
{
	AuxillaryStateStringConverter converter("A Very Long Device Name ON");
	BOOST_CHECK(!converter.Label().has_value());
	BOOST_CHECK(!converter.State().has_value());
}

BOOST_AUTO_TEST_CASE(TestConvert_NoStateToken_ReturnsError)
{
	AuxillaryStateStringConverter converter("Filter Pump  XYZ");
	BOOST_CHECK(!converter.Label().has_value());
	BOOST_CHECK(!converter.State().has_value());
}

// =============================================================================
// Assignment operators
// =============================================================================

BOOST_AUTO_TEST_CASE(TestAssign_StringOperator)
{
	AuxillaryStateStringConverter converter;
	converter = std::string("Pool Light   ON");

	BOOST_REQUIRE(converter.Label().has_value());
	BOOST_REQUIRE(converter.State().has_value());
	BOOST_CHECK_EQUAL(converter.Label().value(), "Pool Light");
	BOOST_CHECK(converter.State().value() == AuxillaryStatuses::On);
}

BOOST_AUTO_TEST_CASE(TestAssign_CopyConstructor)
{
	AuxillaryStateStringConverter original("Waterfall   OFF");
	AuxillaryStateStringConverter copy(original);

	BOOST_REQUIRE(copy.Label().has_value());
	BOOST_CHECK_EQUAL(copy.Label().value(), "Waterfall");
	BOOST_CHECK(copy.State().value() == AuxillaryStatuses::Off);
}

BOOST_AUTO_TEST_CASE(TestAssign_MoveConstructor)
{
	AuxillaryStateStringConverter original("Spillway    ENA");
	AuxillaryStateStringConverter moved(std::move(original));

	BOOST_REQUIRE(moved.Label().has_value());
	BOOST_CHECK_EQUAL(moved.Label().value(), "Spillway");
	BOOST_CHECK(moved.State().value() == AuxillaryStatuses::Enabled);
}

BOOST_AUTO_TEST_SUITE_END()
