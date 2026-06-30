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
#include "jandy/messages/jandy_message_message_long.h"

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

// -----------------------------------------------------------------------------
// ExtractTrailingDisplayLine: panel NUL padding must NOT surface as '?', leading
// spaces (the panel's centring) must be preserved, and a legitimate trailing
// space must round-trip unchanged.
// -----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(TestExtractTrailingDisplayLine_StripsTrailingNulPad)
{
	// frame: 10 02 | dst type | "AB" 00 00 | footer(00 10 03); start_index past header.
	const std::vector<uint8_t> bytes = { 0x10, 0x02, 0x33, 0x04, 'A', 'B', 0x00, 0x00, 0x00, 0x10, 0x03 };
	const auto result = Text::ExtractTrailingDisplayLine(std::span<const uint8_t>(bytes), 4);
	BOOST_CHECK_EQUAL(result, std::string("AB")); // trailing NUL pad stripped, not "AB??".
}

BOOST_AUTO_TEST_CASE(TestExtractTrailingDisplayLine_PreservesLeadingSpacesTrimsTrailing)
{
	// "   Hi" centred with leading spaces, trailing NUL pad.
	const std::vector<uint8_t> bytes = { 0x10, 0x02, 0x33, 0x04, ' ', ' ', ' ', 'H', 'i', 0x00, 0x00, 0x00, 0x10, 0x03 };
	const auto result = Text::ExtractTrailingDisplayLine(std::span<const uint8_t>(bytes), 4);
	BOOST_CHECK_EQUAL(result, std::string("   Hi")); // leading spaces kept, trailing pad gone.
}

BOOST_AUTO_TEST_CASE(TestExtractTrailingDisplayLine_InteriorNulBecomesSpace)
{
	// Two fields separated by NUL column separators: "Menu" 00 00 "Help".
	const std::vector<uint8_t> bytes = { 0x10, 0x02, 0x33, 0x04, 'M', 'e', 'n', 'u', 0x00, 0x00, 'H', 'e', 'l', 'p', 0x00, 0x10, 0x03 };
	const auto result = Text::ExtractTrailingDisplayLine(std::span<const uint8_t>(bytes), 4);
	BOOST_CHECK_EQUAL(result, std::string("Menu  Help")); // interior NUL -> space, not '?'.
}

BOOST_AUTO_TEST_CASE(TestExtractTrailingDisplayLine_KeepsTrailingSpaceAndLiteralQuestionMark)
{
	// A real trailing space (0x20) is printable and must survive; a literal '?' (0x3F)
	// on the wire is also printable and must NOT be mistaken for sanitised pad.
	const std::vector<uint8_t> bytes = { 0x10, 0x02, 0x33, 0x04, 'O', 'k', '?', ' ', 0x00, 0x10, 0x03 };
	const auto result = Text::ExtractTrailingDisplayLine(std::span<const uint8_t>(bytes), 4);
	BOOST_CHECK_EQUAL(result, std::string("Ok? ")); // trailing NUL stripped; space + '?' kept.
}

BOOST_AUTO_TEST_CASE(TestExtractTrailingDisplayLine_HostileControlStillSanitised)
{
	// An interior ESC must still be neutralised to '?' (security: not whitespace pad).
	const std::vector<uint8_t> bytes = { 0x10, 0x02, 0x33, 0x04, 'H', 'o', 0x1B, 'm', 'e', 0x00, 0x10, 0x03 };
	const auto result = Text::ExtractTrailingDisplayLine(std::span<const uint8_t>(bytes), 4);
	BOOST_CHECK_EQUAL(result, std::string("Ho?me"));
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

BOOST_AUTO_TEST_CASE(TestMessageLong_OneTouchTitle_NulPadNotRendered)
{
	// Reconstructs the real "More OneTouch" row (onetouch_setpoint_edit.cap): line id
	// 0x0A, the centred cell " More OneTouch", then two NUL pad bytes.  The pad must
	// disappear, not render as "More OneTouch??".
	std::vector<uint8_t> payload = {
		0x0A, // line id
		' ', 'M', 'o', 'r', 'e', ' ', 'O', 'n', 'e', 'T', 'o', 'u', 'c', 'h',
		0x00, 0x00 // panel NUL pad
	};
	auto pkt = MakePacket(0x41, 0x04, payload);

	JandyMessage_MessageLong msg;
	BOOST_REQUIRE(msg.DeserializeContents(std::span<const uint8_t>(pkt)));
	BOOST_CHECK_EQUAL(msg.LineId(), static_cast<uint8_t>(0x0A));
	BOOST_CHECK_EQUAL(msg.Line(), std::string(" More OneTouch"));
}

BOOST_AUTO_TEST_CASE(TestMessageLong_SystemTitle_CentredWithNulPad)
{
	// Reconstructs the real "System" title row: line id 0x0B, five leading spaces that
	// centre the word, then a NUL/LF/NUL pad run.  Centring (leading spaces) survives;
	// the trailing pad does not become "?????".
	std::vector<uint8_t> payload = {
		0x0B, // line id
		' ', ' ', ' ', ' ', ' ', 'S', 'y', 's', 't', 'e', 'm',
		0x00, 0x0A, 0x00, 0x00, 0x00 // panel pad run
	};
	auto pkt = MakePacket(0x41, 0x04, payload);

	JandyMessage_MessageLong msg;
	BOOST_REQUIRE(msg.DeserializeContents(std::span<const uint8_t>(pkt)));
	BOOST_CHECK_EQUAL(msg.LineId(), static_cast<uint8_t>(0x0B));
	BOOST_CHECK_EQUAL(msg.Line(), std::string("     System"));
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
