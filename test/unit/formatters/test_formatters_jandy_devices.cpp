#include <format>
#include <sstream>
#include <string>

#include <boost/test/unit_test.hpp>

#include "jandy/formatters/jandy_device_formatters.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Devices;

BOOST_AUTO_TEST_SUITE(Formatters_JandyDevicesTestSuite)

// =============================================================================
// std::formatter<JandyDeviceId>
// =============================================================================

BOOST_AUTO_TEST_CASE(TestFormat_JandyDeviceId_HexOutput)
{
	JandyDeviceId id(0x33);
	auto result = std::format("{}", id);
	BOOST_CHECK_EQUAL(result, "0x33");
}

BOOST_AUTO_TEST_CASE(TestFormat_JandyDeviceId_ZeroPadded)
{
	JandyDeviceId id(0x01);
	auto result = std::format("{}", id);
	BOOST_CHECK_EQUAL(result, "0x01");
}

BOOST_AUTO_TEST_CASE(TestFormat_JandyDeviceId_MaxValue)
{
	JandyDeviceId id(0xFF);
	auto result = std::format("{}", id);
	BOOST_CHECK_EQUAL(result, "0xff");
}

BOOST_AUTO_TEST_CASE(TestFormat_JandyDeviceId_Zero)
{
	JandyDeviceId id(0x00);
	auto result = std::format("{}", id);
	BOOST_CHECK_EQUAL(result, "0x00");
}

// =============================================================================
// std::formatter<JandyDeviceType>
// =============================================================================

BOOST_AUTO_TEST_CASE(TestFormat_JandyDeviceType_ShowsDeviceId)
{
	JandyDeviceType device_type(JandyDeviceId(0x50));
	auto result = std::format("{}", device_type);
	// The formatter outputs the device Id via the JandyDeviceId formatter
	BOOST_CHECK_EQUAL(result, "0x50");
}

// =============================================================================
// ostream operators
// =============================================================================

BOOST_AUTO_TEST_CASE(TestOstream_JandyDeviceId)
{
	JandyDeviceId id(0x33);
	std::ostringstream oss;
	oss << id;
	BOOST_CHECK_EQUAL(oss.str(), "0x33");
}

BOOST_AUTO_TEST_CASE(TestOstream_JandyDeviceType)
{
	JandyDeviceType device_type(JandyDeviceId(0x50));
	std::ostringstream oss;
	oss << device_type;
	BOOST_CHECK_EQUAL(oss.str(), "0x50");
}

BOOST_AUTO_TEST_SUITE_END()
