#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/messages/jandy_message_text_helpers.h"

#include "jandy/messages/iaq/iaq_message_aux_status.h"
#include "jandy/messages/iaq/iaq_message_device_id.h"
#include "jandy/messages/iaq/iaq_message_page_message.h"
#include "jandy/messages/iaq/iaq_message_title_message.h"
#include "jandy/messages/jandy_message_message.h"

using namespace AqualinkAutomate::Messages;

namespace
{
	std::vector<uint8_t> MakePacket(uint8_t dest, uint8_t msg_type, const std::vector<uint8_t>& payload)
	{
		std::vector<uint8_t> pkt;
		pkt.push_back(0x10); // DLE
		pkt.push_back(0x02); // STX
		pkt.push_back(dest);
		pkt.push_back(msg_type);
		pkt.insert(pkt.end(), payload.begin(), payload.end());
		pkt.push_back(0x00); // dummy checksum
		pkt.push_back(0x10); // DLE
		pkt.push_back(0x03); // ETX
		return pkt;
	}
}

// =============================================================================
// Text helper unit tests
// =============================================================================

BOOST_AUTO_TEST_SUITE(JandyMessageTextHelpersTestSuite)

BOOST_AUTO_TEST_CASE(TestSanitisePrintableAsciiByte_PassesPrintable)
{
	BOOST_CHECK_EQUAL(Text::SanitisePrintableAsciiByte(0x20), ' ');
	BOOST_CHECK_EQUAL(Text::SanitisePrintableAsciiByte('A'), 'A');
	BOOST_CHECK_EQUAL(Text::SanitisePrintableAsciiByte('~'), '~');
	BOOST_CHECK_EQUAL(Text::SanitisePrintableAsciiByte(0x7E), '~');
}

BOOST_AUTO_TEST_CASE(TestSanitisePrintableAsciiByte_ReplacesControlAndHighBytes)
{
	BOOST_CHECK_EQUAL(Text::SanitisePrintableAsciiByte(0x00), '?'); // NUL
	BOOST_CHECK_EQUAL(Text::SanitisePrintableAsciiByte(0x1B), '?'); // ESC
	BOOST_CHECK_EQUAL(Text::SanitisePrintableAsciiByte(0x0A), '?'); // LF
	BOOST_CHECK_EQUAL(Text::SanitisePrintableAsciiByte(0x7F), '?'); // DEL
	BOOST_CHECK_EQUAL(Text::SanitisePrintableAsciiByte(0xFF), '?'); // high bit
}

BOOST_AUTO_TEST_CASE(TestReadU16LE)
{
	const std::vector<uint8_t> bytes = { 0x90, 0x01 }; // low=0x90, high=0x01
	BOOST_CHECK_EQUAL(Text::ReadU16LE(std::span<const uint8_t>(bytes), 0), static_cast<uint16_t>(0x0190));
}

BOOST_AUTO_TEST_CASE(TestExtractTrailingAsciiPayload_TooShortReturnsEmpty)
{
	// start_index + footer >= size -> empty.
	const std::vector<uint8_t> bytes = { 0x10, 0x02, 0x33, 0x2D, 0x00, 0x10, 0x03 }; // size 7
	const auto result = Text::ExtractTrailingAsciiPayload(std::span<const uint8_t>(bytes), 4);
	BOOST_CHECK(result.empty());
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// Sanitisation at the decode boundary (regression for "Wire-derived ASCII
// strings copied into message fields and logged without sanitisation").
// =============================================================================

BOOST_AUTO_TEST_SUITE(JandyMessageTextSanitisationTestSuite)

BOOST_AUTO_TEST_CASE(TestTitleMessage_SanitisesControlBytes)
{
	// "Ho<ESC>me" with an embedded escape byte; the escape must be neutralised.
	std::vector<uint8_t> payload = { 'H', 'o', 0x1B, 'm', 'e' };
	auto pkt = MakePacket(0x33, 0x2D, payload);

	IAQMessage_TitleMessage msg;
	BOOST_REQUIRE(msg.DeserializeContents(std::span<const uint8_t>(pkt)));
	BOOST_CHECK_EQUAL(msg.Title(), std::string("Ho?me"));
}

BOOST_AUTO_TEST_CASE(TestPageMessage_SanitisesControlBytes)
{
	// LineId 0x05 then "A\nB" with an embedded line feed.
	std::vector<uint8_t> payload = { 0x05, 'A', 0x0A, 'B' };
	auto pkt = MakePacket(0x33, 0x25, payload);

	IAQMessage_PageMessage msg;
	BOOST_REQUIRE(msg.DeserializeContents(std::span<const uint8_t>(pkt)));
	BOOST_CHECK_EQUAL(msg.LineId(), static_cast<uint8_t>(5));
	BOOST_CHECK_EQUAL(msg.Line(), std::string("A?B"));
}

BOOST_AUTO_TEST_CASE(TestJandyMessage_Message_SanitisesControlBytes)
{
	std::vector<uint8_t> payload = { 'O', 'K', 0x07, '!' }; // bell byte
	auto pkt = MakePacket(0x00, 0x03, payload);

	JandyMessage_Message msg;
	BOOST_REQUIRE(msg.DeserializeContents(std::span<const uint8_t>(pkt)));
	BOOST_CHECK_EQUAL(msg.Line(), std::string("OK?!"));
}

BOOST_AUTO_TEST_CASE(TestAuxStatus_SanitisesDeviceName)
{
	// One device whose name contains a control byte.
	std::vector<uint8_t> payload = {
		0x01,                   // num_devices
		0x03,                   // device index
		0x01,                   // status ON
		0x08,                   // type
		0x00, 0x00,             // pad
		0x04,                   // name_len = 4
		'P', 0x1B, 'm', 'p'     // "P<ESC>mp"
	};
	auto pkt = MakePacket(0x33, 0x72, payload);

	IAQMessage_AuxStatus msg;
	BOOST_REQUIRE(msg.DeserializeContents(std::span<const uint8_t>(pkt)));
	BOOST_REQUIRE_EQUAL(msg.Devices().size(), static_cast<size_t>(1));
	BOOST_CHECK_EQUAL(msg.Devices()[0].name, std::string("P?mp"));
}

BOOST_AUTO_TEST_CASE(TestDeviceId_SanitisesNonPrintableBeforeNul)
{
	// Id text "AB<0x01>C" then NUL terminator; the control byte is sanitised but
	// the decode still stops at the NUL.
	std::vector<uint8_t> frame = {
		0x10, 0x02, 0xA3, 0x51,
		'A', 'B', 0x01, 'C', 0x00,
		0x00,        // checksum placeholder
		0x10, 0x03
	};

	IAQMessage_DeviceId msg;
	BOOST_REQUIRE(msg.DeserializeContents(std::span<const uint8_t>(frame)));
	BOOST_CHECK_EQUAL(msg.DeviceId(), std::string("AB?C"));
}

BOOST_AUTO_TEST_SUITE_END()
