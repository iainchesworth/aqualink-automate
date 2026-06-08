#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

#include "mqtt/mqtt_payload_parsing.h"

using namespace AqualinkAutomate;
using AqualinkAutomate::Mqtt::PayloadParsing::ParsePayloadNumber;
using AqualinkAutomate::Mqtt::PayloadParsing::ParsePayloadString;
using AqualinkAutomate::Mqtt::PayloadParsing::SanitiseForLog;

namespace
{
	// Compare uint8_t results widened to unsigned so a failure prints a number, not a char glyph.
	constexpr unsigned int AsUInt(uint8_t value)
	{
		return static_cast<unsigned int>(value);
	}
}

//=============================================================================
// ParsePayloadNumber - JSON number payloads
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_Mqtt_ParsePayloadNumber_JsonNumber)

BOOST_AUTO_TEST_CASE(Test_ValidUint8Number_Parsed)
{
	nlohmann::json payload = 50;
	BOOST_CHECK_EQUAL(AsUInt(ParsePayloadNumber<uint8_t>(payload)), 50u);
}

BOOST_AUTO_TEST_CASE(Test_ValidDoubleNumber_Parsed)
{
	nlohmann::json payload = 28.5;
	BOOST_CHECK_CLOSE(ParsePayloadNumber<double>(payload, 0.0), 28.5, 0.0001);
}

BOOST_AUTO_TEST_CASE(Test_Uint8MaxBoundary_Parsed)
{
	nlohmann::json payload = 255;
	BOOST_CHECK_EQUAL(AsUInt(ParsePayloadNumber<uint8_t>(payload)), 255u);
}

// Regression: a broker-supplied JSON number above uint8_t's range must NOT wrap/truncate
// (300 % 256 == 44 would previously be accepted) - it must be rejected to the default.
BOOST_AUTO_TEST_CASE(Test_Uint8OverRangeNumber_RejectedNotWrapped)
{
	nlohmann::json payload = 300;
	BOOST_CHECK_EQUAL(AsUInt(ParsePayloadNumber<uint8_t>(payload, 7u)), 7u);

	nlohmann::json payload2 = 256;
	BOOST_CHECK_EQUAL(AsUInt(ParsePayloadNumber<uint8_t>(payload2, 0u)), 0u);
}

BOOST_AUTO_TEST_CASE(Test_Uint8NegativeNumber_Rejected)
{
	nlohmann::json payload = -5;
	BOOST_CHECK_EQUAL(AsUInt(ParsePayloadNumber<uint8_t>(payload, 9u)), 9u);
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// ParsePayloadNumber - {"raw": "..."} and plain string payloads
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_Mqtt_ParsePayloadNumber_StringForms)

BOOST_AUTO_TEST_CASE(Test_RawStringNumber_Parsed)
{
	nlohmann::json payload = {{"raw", "42"}};
	BOOST_CHECK_EQUAL(AsUInt(ParsePayloadNumber<uint8_t>(payload)), 42u);
}

BOOST_AUTO_TEST_CASE(Test_PlainStringNumber_Parsed)
{
	nlohmann::json payload = "75";
	BOOST_CHECK_EQUAL(AsUInt(ParsePayloadNumber<uint8_t>(payload)), 75u);
}

// Regression: an out-of-range string value must be rejected (from_chars reports
// result_out_of_range), not truncated via static_cast<uint8_t>(std::stoi(...)).
BOOST_AUTO_TEST_CASE(Test_RawStringOverRange_RejectedNotTruncated)
{
	nlohmann::json payload = {{"raw", "300"}};
	BOOST_CHECK_EQUAL(AsUInt(ParsePayloadNumber<uint8_t>(payload, 0u)), 0u);
}

BOOST_AUTO_TEST_CASE(Test_PlainStringOverRange_RejectedNotTruncated)
{
	nlohmann::json payload = "1000";
	BOOST_CHECK_EQUAL(AsUInt(ParsePayloadNumber<uint8_t>(payload, 3u)), 3u);
}

// Regression: a non-numeric string must return the default instead of throwing
// std::invalid_argument (the old std::stoi path threw on the inbound MQTT hot path).
BOOST_AUTO_TEST_CASE(Test_NonNumericString_ReturnsDefault_NoThrow)
{
	nlohmann::json payload = "not-a-number";
	BOOST_CHECK_NO_THROW(ParsePayloadNumber<double>(payload, -1.0));
	BOOST_CHECK_CLOSE(ParsePayloadNumber<double>(payload, -1.0), -1.0, 0.0001);
}

BOOST_AUTO_TEST_CASE(Test_RawStringDouble_Parsed)
{
	nlohmann::json payload = {{"raw", "28.5"}};
	BOOST_CHECK_CLOSE(ParsePayloadNumber<double>(payload, 0.0), 28.5, 0.0001);
}

BOOST_AUTO_TEST_CASE(Test_EmptyObject_ReturnsDefault)
{
	nlohmann::json payload = nlohmann::json::object();
	BOOST_CHECK_EQUAL(AsUInt(ParsePayloadNumber<uint8_t>(payload, 11u)), 11u);
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// ParsePayloadString
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_Mqtt_ParsePayloadString)

BOOST_AUTO_TEST_CASE(Test_PlainString_Returned)
{
	nlohmann::json payload = "ON";
	BOOST_CHECK_EQUAL(ParsePayloadString(payload), "ON");
}

BOOST_AUTO_TEST_CASE(Test_RawString_Returned)
{
	nlohmann::json payload = {{"raw", "OFF"}};
	BOOST_CHECK_EQUAL(ParsePayloadString(payload), "OFF");
}

BOOST_AUTO_TEST_CASE(Test_RawNonString_ReturnsEmpty)
{
	// A non-string "raw" must not throw; it falls through to empty.
	nlohmann::json payload = {{"raw", 5}};
	BOOST_CHECK_NO_THROW(ParsePayloadString(payload));
	BOOST_CHECK_EQUAL(ParsePayloadString(payload), "");
}

BOOST_AUTO_TEST_CASE(Test_NumberPayload_ReturnsEmpty)
{
	nlohmann::json payload = 5;
	BOOST_CHECK_EQUAL(ParsePayloadString(payload), "");
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// SanitiseForLog
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_Mqtt_SanitiseForLog)

BOOST_AUTO_TEST_CASE(Test_PrintableAsciiPassesThrough)
{
	BOOST_CHECK_EQUAL(SanitiseForLog("ON"), "ON");
	BOOST_CHECK_EQUAL(SanitiseForLog("pool spillover-42.5"), "pool spillover-42.5");
}

BOOST_AUTO_TEST_CASE(Test_ControlBytesReplaced)
{
	std::string value = "a\x01\x1f\tb\x7f";
	BOOST_CHECK_EQUAL(SanitiseForLog(value), "a???b?");
}

BOOST_AUTO_TEST_CASE(Test_OversizedPayloadTruncated)
{
	const std::string value(200, 'x');
	const auto sanitised = SanitiseForLog(value);

	// 64 retained characters plus a "..." ellipsis marker.
	BOOST_CHECK_EQUAL(sanitised.size(), Mqtt::PayloadParsing::MAX_LOGGED_PAYLOAD_LENGTH + 3U);
	BOOST_CHECK(sanitised.ends_with("..."));
}

BOOST_AUTO_TEST_CASE(Test_EmptyStringStaysEmpty)
{
	BOOST_CHECK_EQUAL(SanitiseForLog(""), "");
}

BOOST_AUTO_TEST_SUITE_END()
