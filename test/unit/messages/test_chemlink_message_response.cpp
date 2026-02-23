#include <cstdint>
#include <string>
#include <span>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <magic_enum/magic_enum.hpp>

#include "jandy/messages/jandy_message_constants.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/chemlink/chemlink_message_response.h"
#include "jandy/utility/jandy_checksum.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Messages;

BOOST_AUTO_TEST_SUITE(ChemlinkMessage_ResponseTestSuite)

// =============================================================================
// Construction Defaults
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_DefaultValues)
{
	ChemlinkMessage_Response message;

	BOOST_CHECK(message.DataTag() == ChemlinkDataTag::Unknown);
	BOOST_CHECK_EQUAL(message.RawValue(), static_cast<uint16_t>(0));
}

BOOST_AUTO_TEST_CASE(TestConstruction_MessageId)
{
	ChemlinkMessage_Response message;

	BOOST_CHECK(message.Id() == JandyMessageIds::Chemlink_Response);
}

// =============================================================================
// Deserialization: Each ChemlinkDataTag
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDeserialize_ORP_Tag)
{
	// Build message bytes: DLE STX DEST CMD TAG LOW HIGH CKSUM DLE ETX
	// Index 4 = DataTag, Index 5 = ValueLow, Index 6 = ValueHigh
	std::vector<uint8_t> message_bytes = {
		HEADER_BYTE_DLE, HEADER_BYTE_STX,
		0x00,                                                    // destination
		magic_enum::enum_integer(JandyMessageIds::Chemlink_Response),
		0x02,                                                    // ORP tag
		0x64,                                                    // low byte (100)
		0x00,                                                    // high byte (0)
		0x00,                                                    // checksum placeholder
		HEADER_BYTE_DLE, HEADER_BYTE_ETX
	};
	message_bytes[7] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(
		message_bytes.begin(), message_bytes.begin() + 7);

	ChemlinkMessage_Response message;
	BOOST_REQUIRE(message.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
	BOOST_CHECK(message.DataTag() == ChemlinkDataTag::ORP);
	BOOST_CHECK_EQUAL(message.RawValue(), static_cast<uint16_t>(100));
}

BOOST_AUTO_TEST_CASE(TestDeserialize_pH_Tag)
{
	std::vector<uint8_t> message_bytes = {
		HEADER_BYTE_DLE, HEADER_BYTE_STX,
		0x00,
		magic_enum::enum_integer(JandyMessageIds::Chemlink_Response),
		0x03,    // pH tag
		0x4B,    // low byte (75)
		0x00,    // high byte (0)
		0x00,
		HEADER_BYTE_DLE, HEADER_BYTE_ETX
	};
	message_bytes[7] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(
		message_bytes.begin(), message_bytes.begin() + 7);

	ChemlinkMessage_Response message;
	BOOST_REQUIRE(message.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
	BOOST_CHECK(message.DataTag() == ChemlinkDataTag::pH);
	BOOST_CHECK_EQUAL(message.RawValue(), static_cast<uint16_t>(75));
}

BOOST_AUTO_TEST_CASE(TestDeserialize_pHFeeder_Tag)
{
	std::vector<uint8_t> message_bytes = {
		HEADER_BYTE_DLE, HEADER_BYTE_STX,
		0x00,
		magic_enum::enum_integer(JandyMessageIds::Chemlink_Response),
		0x08,    // pHFeeder tag
		0x00,    // low byte
		0x00,    // high byte
		0x00,
		HEADER_BYTE_DLE, HEADER_BYTE_ETX
	};
	message_bytes[7] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(
		message_bytes.begin(), message_bytes.begin() + 7);

	ChemlinkMessage_Response message;
	BOOST_REQUIRE(message.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
	BOOST_CHECK(message.DataTag() == ChemlinkDataTag::pHFeeder);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_ORPFeeder_Tag)
{
	std::vector<uint8_t> message_bytes = {
		HEADER_BYTE_DLE, HEADER_BYTE_STX,
		0x00,
		magic_enum::enum_integer(JandyMessageIds::Chemlink_Response),
		0x18,    // ORPFeeder tag
		0x01,    // low byte
		0x00,    // high byte
		0x00,
		HEADER_BYTE_DLE, HEADER_BYTE_ETX
	};
	message_bytes[7] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(
		message_bytes.begin(), message_bytes.begin() + 7);

	ChemlinkMessage_Response message;
	BOOST_REQUIRE(message.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
	BOOST_CHECK(message.DataTag() == ChemlinkDataTag::ORPFeeder);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_Unknown_Tag_Fallback)
{
	std::vector<uint8_t> message_bytes = {
		HEADER_BYTE_DLE, HEADER_BYTE_STX,
		0x00,
		magic_enum::enum_integer(JandyMessageIds::Chemlink_Response),
		0x99,    // Unknown tag value
		0x0A,    // low byte
		0x00,    // high byte
		0x00,
		HEADER_BYTE_DLE, HEADER_BYTE_ETX
	};
	message_bytes[7] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(
		message_bytes.begin(), message_bytes.begin() + 7);

	ChemlinkMessage_Response message;
	BOOST_REQUIRE(message.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
	BOOST_CHECK(message.DataTag() == ChemlinkDataTag::Unknown);
	BOOST_CHECK_EQUAL(message.RawValue(), static_cast<uint16_t>(10));
}

// =============================================================================
// 16-bit LE RawValue
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDeserialize_RawValue_HighAndLow)
{
	// RawValue = high * 256 + low = 0x01 * 256 + 0x90 = 400
	std::vector<uint8_t> message_bytes = {
		HEADER_BYTE_DLE, HEADER_BYTE_STX,
		0x00,
		magic_enum::enum_integer(JandyMessageIds::Chemlink_Response),
		0x02,    // ORP
		0x90,    // low byte (144)
		0x01,    // high byte (1)
		0x00,
		HEADER_BYTE_DLE, HEADER_BYTE_ETX
	};
	message_bytes[7] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(
		message_bytes.begin(), message_bytes.begin() + 7);

	ChemlinkMessage_Response message;
	BOOST_REQUIRE(message.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
	BOOST_CHECK_EQUAL(message.RawValue(), static_cast<uint16_t>(1 * 256 + 144));
}

BOOST_AUTO_TEST_CASE(TestDeserialize_RawValue_MaxValue)
{
	// RawValue = 0xFF * 256 + 0xFF = 65535
	std::vector<uint8_t> message_bytes = {
		HEADER_BYTE_DLE, HEADER_BYTE_STX,
		0x00,
		magic_enum::enum_integer(JandyMessageIds::Chemlink_Response),
		0x02,    // ORP
		0xFF,    // low byte
		0xFF,    // high byte
		0x00,
		HEADER_BYTE_DLE, HEADER_BYTE_ETX
	};
	message_bytes[7] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(
		message_bytes.begin(), message_bytes.begin() + 7);

	ChemlinkMessage_Response message;
	BOOST_REQUIRE(message.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
	BOOST_CHECK_EQUAL(message.RawValue(), static_cast<uint16_t>(0xFFFF));
}

BOOST_AUTO_TEST_CASE(TestDeserialize_RawValue_Zero)
{
	std::vector<uint8_t> message_bytes = {
		HEADER_BYTE_DLE, HEADER_BYTE_STX,
		0x00,
		magic_enum::enum_integer(JandyMessageIds::Chemlink_Response),
		0x03,    // pH
		0x00,    // low byte
		0x00,    // high byte
		0x00,
		HEADER_BYTE_DLE, HEADER_BYTE_ETX
	};
	message_bytes[7] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(
		message_bytes.begin(), message_bytes.begin() + 7);

	ChemlinkMessage_Response message;
	BOOST_REQUIRE(message.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
	BOOST_CHECK_EQUAL(message.RawValue(), static_cast<uint16_t>(0));
}

// =============================================================================
// Partial Messages
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDeserialize_TagOnly_NoValueBytes)
{
	// Message with only up to Index_DataTag (5 bytes: 0..4), no value bytes
	std::vector<uint8_t> content = { 0x00, 0x21, 0x00, 0x00, 0x02 };

	ChemlinkMessage_Response message;
	BOOST_CHECK(message.DeserializeContents(content));
	BOOST_CHECK(message.DataTag() == ChemlinkDataTag::ORP);
	// RawValue should remain at default (0) since no value bytes present
	BOOST_CHECK_EQUAL(message.RawValue(), static_cast<uint16_t>(0));
}

BOOST_AUTO_TEST_CASE(TestDeserialize_LowByteOnly_NoHighByte)
{
	// Message with Index_ValueLow present but not Index_ValueHigh
	std::vector<uint8_t> content = { 0x00, 0x21, 0x00, 0x00, 0x02, 0x42 };

	ChemlinkMessage_Response message;
	BOOST_CHECK(message.DeserializeContents(content));
	BOOST_CHECK(message.DataTag() == ChemlinkDataTag::ORP);
	BOOST_CHECK_EQUAL(message.RawValue(), static_cast<uint16_t>(0x42));
}

BOOST_AUTO_TEST_CASE(TestDeserialize_TooShort_Fails)
{
	// Message too short to contain DataTag
	std::vector<uint8_t> content = { 0x00, 0x21, 0x00, 0x00 };

	ChemlinkMessage_Response message;
	BOOST_CHECK_EQUAL(false, message.DeserializeContents(content));
}

// =============================================================================
// SerializeContents
// =============================================================================

BOOST_AUTO_TEST_CASE(TestSerializeContents_ReturnsFalse)
{
	ChemlinkMessage_Response message;
	std::vector<uint8_t> output;
	BOOST_CHECK_EQUAL(false, message.SerializeContents(output));
}

// =============================================================================
// ToString
// =============================================================================

BOOST_AUTO_TEST_CASE(TestToString_ContainsTagNameAndValue)
{
	ChemlinkMessage_Response message;
	std::string result = message.ToString();
	BOOST_CHECK(result.find("Unknown") != std::string::npos);
	BOOST_CHECK(result.find("0") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestToString_AfterDeserialization)
{
	std::vector<uint8_t> content = { 0x00, 0x21, 0x00, 0x00, 0x02, 0x64, 0x00 };

	ChemlinkMessage_Response message;
	BOOST_REQUIRE(message.DeserializeContents(content));

	std::string result = message.ToString();
	BOOST_CHECK(result.find("ORP") != std::string::npos);
	BOOST_CHECK(result.find("100") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
