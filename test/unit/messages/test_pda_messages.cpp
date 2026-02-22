#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/messages/pda/pda_message_clear.h"
#include "jandy/messages/pda/pda_message_shiftlines.h"
#include "jandy/messages/pda/pda_message_highlight_chars.h"
#include "jandy/messages/jandy_message_display_update.h"

using namespace AqualinkAutomate::Messages;

namespace
{
	std::vector<uint8_t> MakePacket(uint8_t dest, uint8_t msg_type, const std::vector<uint8_t>& payload)
	{
		std::vector<uint8_t> pkt;
		pkt.push_back(0x10); pkt.push_back(0x02);
		pkt.push_back(dest); pkt.push_back(msg_type);
		pkt.insert(pkt.end(), payload.begin(), payload.end());
		pkt.push_back(0x00); pkt.push_back(0x10); pkt.push_back(0x03);
		return pkt;
	}
}

// =============================================================================
// PDAMessage_Clear
// =============================================================================

BOOST_AUTO_TEST_SUITE(PDAMessage_Clear_TestSuite)

BOOST_AUTO_TEST_CASE(TestDeserialize_AlwaysReturnsTrue)
{
	PDAMessage_Clear msg;
	auto pkt = MakePacket(0x60, 0x05, {});
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));
	BOOST_CHECK(result);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_EmptyData_ReturnsTrue)
{
	PDAMessage_Clear msg;
	std::vector<uint8_t> empty;
	auto result = msg.DeserializeContents(std::span<const uint8_t>(empty));
	BOOST_CHECK(result);
}

BOOST_AUTO_TEST_CASE(TestSerialize_ReturnsFalse)
{
	PDAMessage_Clear msg;
	std::vector<uint8_t> bytes;
	BOOST_CHECK(!msg.SerializeContents(bytes));
}

BOOST_AUTO_TEST_CASE(TestToString_NotEmpty)
{
	PDAMessage_Clear msg;
	auto str = msg.ToString();
	BOOST_CHECK(!str.empty());
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// PDAMessage_ShiftLines
// =============================================================================

BOOST_AUTO_TEST_SUITE(PDAMessage_ShiftLines_TestSuite)

BOOST_AUTO_TEST_CASE(TestConstruction_Defaults)
{
	PDAMessage_ShiftLines msg;
	BOOST_CHECK_EQUAL(msg.FirstLineId(), 0);
	BOOST_CHECK_EQUAL(msg.LastLineId(), 0);
	BOOST_CHECK_EQUAL(msg.LineShift(), 0);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_Valid)
{
	PDAMessage_ShiftLines msg;
	// Index_FirstLineId=4, Index_LastLineId=5, Index_LineShift=6
	auto pkt = MakePacket(0x60, 0x09, { 2, 8, 1 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK_EQUAL(msg.FirstLineId(), 2);
	BOOST_CHECK_EQUAL(msg.LastLineId(), 8);
	BOOST_CHECK_EQUAL(msg.LineShift(), 1);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_NegativeShift)
{
	PDAMessage_ShiftLines msg;
	// LineShift is int8_t, so 0xFF = -1
	auto pkt = MakePacket(0x60, 0x09, { 0, 9, 0xFF });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK_EQUAL(msg.FirstLineId(), 0);
	BOOST_CHECK_EQUAL(msg.LastLineId(), 9);
	BOOST_CHECK_EQUAL(msg.LineShift(), -1);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_TooShort_NoFirst)
{
	PDAMessage_ShiftLines msg;
	// size <= Index_FirstLineId(4) → need <=4 bytes
	std::vector<uint8_t> data = { 0x10, 0x02, 0x60, 0x09 };
	auto result = msg.DeserializeContents(std::span<const uint8_t>(data));
	BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_TooShort_NoLast)
{
	PDAMessage_ShiftLines msg;
	// size <= Index_LastLineId(5) → need <=5 bytes
	std::vector<uint8_t> data = { 0x10, 0x02, 0x60, 0x09, 2 };
	auto result = msg.DeserializeContents(std::span<const uint8_t>(data));
	BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_TooShort_NoShift)
{
	PDAMessage_ShiftLines msg;
	// size <= Index_LineShift(6) → need <=6 bytes
	std::vector<uint8_t> data = { 0x10, 0x02, 0x60, 0x09, 2, 8 };
	auto result = msg.DeserializeContents(std::span<const uint8_t>(data));
	BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(TestSerialize_ReturnsFalse)
{
	PDAMessage_ShiftLines msg;
	std::vector<uint8_t> bytes;
	BOOST_CHECK(!msg.SerializeContents(bytes));
}

BOOST_AUTO_TEST_CASE(TestToString_NotEmpty)
{
	PDAMessage_ShiftLines msg;
	auto str = msg.ToString();
	BOOST_CHECK(!str.empty());
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// PDAMessage_HighlightChars
// =============================================================================

BOOST_AUTO_TEST_SUITE(PDAMessage_HighlightChars_TestSuite)

BOOST_AUTO_TEST_CASE(TestConstruction_Defaults)
{
	PDAMessage_HighlightChars msg;
	BOOST_CHECK_EQUAL(msg.LineId(), 0);
	BOOST_CHECK_EQUAL(msg.StartIndex(), 0);
	BOOST_CHECK_EQUAL(msg.StopIndex(), 0);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_Valid)
{
	PDAMessage_HighlightChars msg;
	// Index_LineId=4, Index_StartIndex=5, Index_StopIndex=6
	auto pkt = MakePacket(0x60, 0x10, { 3, 5, 15 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK_EQUAL(msg.LineId(), 3);
	BOOST_CHECK_EQUAL(msg.StartIndex(), 5);
	BOOST_CHECK_EQUAL(msg.StopIndex(), 15);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_ZeroValues)
{
	PDAMessage_HighlightChars msg;
	auto pkt = MakePacket(0x60, 0x10, { 0, 0, 0 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK_EQUAL(msg.LineId(), 0);
	BOOST_CHECK_EQUAL(msg.StartIndex(), 0);
	BOOST_CHECK_EQUAL(msg.StopIndex(), 0);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_TooShort_NoLineId)
{
	PDAMessage_HighlightChars msg;
	std::vector<uint8_t> data = { 0x10, 0x02, 0x60, 0x10 };
	auto result = msg.DeserializeContents(std::span<const uint8_t>(data));
	BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_TooShort_NoStart)
{
	PDAMessage_HighlightChars msg;
	std::vector<uint8_t> data = { 0x10, 0x02, 0x60, 0x10, 3 };
	auto result = msg.DeserializeContents(std::span<const uint8_t>(data));
	BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_TooShort_NoStop)
{
	PDAMessage_HighlightChars msg;
	std::vector<uint8_t> data = { 0x10, 0x02, 0x60, 0x10, 3, 5 };
	auto result = msg.DeserializeContents(std::span<const uint8_t>(data));
	BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(TestSerialize_ReturnsFalse)
{
	PDAMessage_HighlightChars msg;
	std::vector<uint8_t> bytes;
	BOOST_CHECK(!msg.SerializeContents(bytes));
}

BOOST_AUTO_TEST_CASE(TestToString_NotEmpty)
{
	PDAMessage_HighlightChars msg;
	auto str = msg.ToString();
	BOOST_CHECK(!str.empty());
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// JandyMessage_DisplayUpdate
// =============================================================================

BOOST_AUTO_TEST_SUITE(JandyMessage_DisplayUpdate_TestSuite)

BOOST_AUTO_TEST_CASE(TestDeserialize_AlwaysReturnsTrue)
{
	JandyMessage_DisplayUpdate msg;
	auto pkt = MakePacket(0x41, 0x1F, {});
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));
	BOOST_CHECK(result);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_EmptyData_ReturnsTrue)
{
	JandyMessage_DisplayUpdate msg;
	std::vector<uint8_t> empty;
	auto result = msg.DeserializeContents(std::span<const uint8_t>(empty));
	BOOST_CHECK(result);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_WithPayload_ReturnsTrue)
{
	JandyMessage_DisplayUpdate msg;
	auto pkt = MakePacket(0x41, 0x1F, { 0x01, 0x02, 0x03 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));
	BOOST_CHECK(result);
}

BOOST_AUTO_TEST_CASE(TestSerialize_ReturnsFalse)
{
	JandyMessage_DisplayUpdate msg;
	std::vector<uint8_t> bytes;
	BOOST_CHECK(!msg.SerializeContents(bytes));
}

BOOST_AUTO_TEST_CASE(TestToString_NotEmpty)
{
	JandyMessage_DisplayUpdate msg;
	auto str = msg.ToString();
	BOOST_CHECK(!str.empty());
}

BOOST_AUTO_TEST_SUITE_END()
