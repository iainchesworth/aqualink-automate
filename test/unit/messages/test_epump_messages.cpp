#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/messages/epump/epump_message_rpm.h"
#include "jandy/messages/epump/epump_message_status.h"
#include "jandy/messages/epump/epump_message_watts.h"

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
// EPumpMessage_RPM
// =============================================================================

BOOST_AUTO_TEST_SUITE(EPumpMessage_RPM_TestSuite)

BOOST_AUTO_TEST_CASE(TestConstruction_Defaults)
{
	EPumpMessage_RPM msg;
	BOOST_CHECK_EQUAL(msg.RPM(), 0);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_ValidRPM)
{
	EPumpMessage_RPM msg;

	// RPM formula: (high * 256 + low) / 4
	// Target: 2500 RPM → high*256+low = 10000 → high=39(0x27), low=16(0x10)
	auto pkt = MakePacket(0x50, 0x04, { 0x00, 0x00, 0x10, 0x27 });
	// Indices: [0]=DLE, [1]=STX, [2]=dest, [3]=msg_type, [4]=0x00, [5]=0x00, [6]=RPM_Low(0x10), [7]=RPM_High(0x27)
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK_EQUAL(msg.RPM(), 2500);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_ZeroRPM)
{
	EPumpMessage_RPM msg;
	auto pkt = MakePacket(0x50, 0x04, { 0x00, 0x00, 0x00, 0x00 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK_EQUAL(msg.RPM(), 0);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_MaxRPM)
{
	EPumpMessage_RPM msg;
	// Max: (255 * 256 + 255) / 4 = 65535 / 4 = 16383
	auto pkt = MakePacket(0x50, 0x04, { 0x00, 0x00, 0xFF, 0xFF });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK_EQUAL(msg.RPM(), 16383);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_TooShort)
{
	EPumpMessage_RPM msg;
	// Only payload up to Index_RPM_Low (6), missing Index_RPM_High (7)
	auto pkt = MakePacket(0x50, 0x04, { 0x00, 0x00, 0x10 });
	// Packet size = 4 + 3 + 3 = 10, but we need size > 7 (Index_RPM_High)
	// This packet: [DLE, STX, dest, msg, 0x00, 0x00, 0x10, checksum, DLE, ETX] → size=10
	// Index 7 = checksum byte, size=10 > 7, so this actually passes.
	// Need truly short: only 2 payload bytes → size = 4+2+3 = 9, indices 0-8, size <= 7 is false
	// Let's make a packet with just 1 payload byte:
	auto short_pkt = MakePacket(0x50, 0x04, {});
	// size = 4+0+3 = 7, size <= 7 is true → returns false
	auto result = msg.DeserializeContents(std::span<const uint8_t>(short_pkt));

	BOOST_CHECK(!result);
	BOOST_CHECK_EQUAL(msg.RPM(), 0);
}

BOOST_AUTO_TEST_CASE(TestSerialize_ReturnsFalse)
{
	EPumpMessage_RPM msg;
	std::vector<uint8_t> bytes;
	BOOST_CHECK(!msg.SerializeContents(bytes));
}

BOOST_AUTO_TEST_CASE(TestToString_ContainsRPM)
{
	EPumpMessage_RPM msg;
	auto pkt = MakePacket(0x50, 0x04, { 0x00, 0x00, 0x10, 0x27 });
	msg.DeserializeContents(std::span<const uint8_t>(pkt));

	auto str = msg.ToString();
	BOOST_CHECK(str.find("RPM") != std::string::npos);
	BOOST_CHECK(str.find("2500") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// EPumpMessage_Status
// =============================================================================

BOOST_AUTO_TEST_SUITE(EPumpMessage_Status_TestSuite)

BOOST_AUTO_TEST_CASE(TestConstruction_Defaults)
{
	EPumpMessage_Status msg;
	BOOST_CHECK_EQUAL(msg.SubCommand(), 0);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_ValidSubCommand)
{
	EPumpMessage_Status msg;
	auto pkt = MakePacket(0x50, 0x02, { 0x42 });
	// Index_SubCommand = 4, pkt[4] = 0x42
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK_EQUAL(msg.SubCommand(), 0x42);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_TooShort)
{
	EPumpMessage_Status msg;
	// Need size > Index_SubCommand(4). Empty payload: size = 4+0+3 = 7, 7 > 4 true.
	// But we need size <= 4. Minimum packet is [DLE,STX,dest,msg] with no footer.
	// Actually with MakePacket, even empty payload gives 7 bytes.
	// size <= 4 can't be reached with a valid frame.
	// So let's test with a raw span of 4 bytes (simulating partial data):
	std::vector<uint8_t> short_data = { 0x10, 0x02, 0x50, 0x02 };
	auto result = msg.DeserializeContents(std::span<const uint8_t>(short_data));

	BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(TestSerialize_WritesSubCommand)
{
	EPumpMessage_Status msg;
	auto pkt = MakePacket(0x50, 0x02, { 0xAB });
	msg.DeserializeContents(std::span<const uint8_t>(pkt));

	std::vector<uint8_t> bytes;
	BOOST_CHECK(msg.SerializeContents(bytes));
	BOOST_REQUIRE_EQUAL(bytes.size(), 1u);
	BOOST_CHECK_EQUAL(bytes[0], 0xAB);
}

BOOST_AUTO_TEST_CASE(TestToString_ContainsSubCommand)
{
	EPumpMessage_Status msg;
	auto pkt = MakePacket(0x50, 0x02, { 0x42 });
	msg.DeserializeContents(std::span<const uint8_t>(pkt));

	auto str = msg.ToString();
	BOOST_CHECK(str.find("SubCommand") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// EPumpMessage_Watts
// =============================================================================

BOOST_AUTO_TEST_SUITE(EPumpMessage_Watts_TestSuite)

BOOST_AUTO_TEST_CASE(TestConstruction_Defaults)
{
	EPumpMessage_Watts msg;
	BOOST_CHECK_EQUAL(msg.Watts(), 0);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_ValidWatts)
{
	EPumpMessage_Watts msg;

	// Watts formula: high * 256 + low
	// Target: 1500 → high=5(0x05), low=220(0xDC)
	// Index_Watts_Low=7, Index_Watts_High=8
	auto pkt = MakePacket(0x50, 0x06, { 0x00, 0x00, 0x00, 0xDC, 0x05 });
	// Indices: [4]=0x00, [5]=0x00, [6]=0x00, [7]=0xDC(low), [8]=0x05(high)
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK_EQUAL(msg.Watts(), 1500);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_ZeroWatts)
{
	EPumpMessage_Watts msg;
	auto pkt = MakePacket(0x50, 0x06, { 0x00, 0x00, 0x00, 0x00, 0x00 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK_EQUAL(msg.Watts(), 0);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_MaxWatts)
{
	EPumpMessage_Watts msg;
	// Max: 255*256 + 255 = 65535
	auto pkt = MakePacket(0x50, 0x06, { 0x00, 0x00, 0x00, 0xFF, 0xFF });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK_EQUAL(msg.Watts(), 65535);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_TooShort)
{
	EPumpMessage_Watts msg;
	// Need size > Index_Watts_High(8). Payload of 4 bytes → size = 4+4+3 = 11, indices 0-10
	// pkt[8] = payload[4] which doesn't exist → we need payload < 5 bytes
	auto pkt = MakePacket(0x50, 0x06, { 0x00, 0x00, 0x00, 0xDC });
	// size = 4+4+3 = 11, Index_Watts_High = 8, size(11) > 8 true → but pkt[8] = checksum
	// Actually this passes because the check is message_bytes.size() <= Index_Watts_High (8)
	// 11 <= 8 is false, so it passes. The value at [8] will be the dummy checksum byte.
	// We need size <= 8, so payload of 1 byte → size = 4+1+3 = 8
	auto short_pkt = MakePacket(0x50, 0x06, { 0x00 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(short_pkt));

	BOOST_CHECK(!result);
	BOOST_CHECK_EQUAL(msg.Watts(), 0);
}

BOOST_AUTO_TEST_CASE(TestSerialize_ReturnsFalse)
{
	EPumpMessage_Watts msg;
	std::vector<uint8_t> bytes;
	BOOST_CHECK(!msg.SerializeContents(bytes));
}

BOOST_AUTO_TEST_CASE(TestToString_ContainsWatts)
{
	EPumpMessage_Watts msg;
	auto pkt = MakePacket(0x50, 0x06, { 0x00, 0x00, 0x00, 0xDC, 0x05 });
	msg.DeserializeContents(std::span<const uint8_t>(pkt));

	auto str = msg.ToString();
	BOOST_CHECK(str.find("Watts") != std::string::npos);
	BOOST_CHECK(str.find("1500") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
