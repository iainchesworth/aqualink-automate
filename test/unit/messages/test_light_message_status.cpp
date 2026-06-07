#include <cstdint>
#include <string>
#include <span>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/light/light_message_status.h"

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

	const uint8_t CMD_LIGHT_STATUS = static_cast<uint8_t>(JandyMessageIds::Light_Status);
}

// =============================================================================
// LightMessage_Status
// =============================================================================

BOOST_AUTO_TEST_SUITE(LightMessage_Status_TestSuite)

BOOST_AUTO_TEST_CASE(TestConstruction_Defaults)
{
	LightMessage_Status msg;
	BOOST_CHECK(msg.State() == LightStates::Unknown);
	BOOST_CHECK_EQUAL(msg.IsOn(), false);
	BOOST_CHECK_EQUAL(msg.LightMode(), 0);
}

BOOST_AUTO_TEST_CASE(TestConstruction_MessageId)
{
	LightMessage_Status msg;
	BOOST_CHECK(msg.Id() == JandyMessageIds::Light_Status);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_On_WithMode)
{
	LightMessage_Status msg;
	// Index_State=4, Index_Mode=5. On = 0x01, mode = 0x07.
	auto pkt = MakePacket(0xF0, CMD_LIGHT_STATUS, { 0x01, 0x07 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK(msg.State() == LightStates::On);
	BOOST_CHECK_EQUAL(msg.IsOn(), true);
	BOOST_CHECK_EQUAL(msg.LightMode(), 0x07);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_Off)
{
	LightMessage_Status msg;
	auto pkt = MakePacket(0xF1, CMD_LIGHT_STATUS, { 0x00, 0x00 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK(msg.State() == LightStates::Off);
	BOOST_CHECK_EQUAL(msg.IsOn(), false);
	BOOST_CHECK_EQUAL(msg.LightMode(), 0x00);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_StateOnly_NoModeByte)
{
	LightMessage_Status msg;
	// Only the state byte present (no mode byte); mode stays at default.
	auto pkt = MakePacket(0xF0, CMD_LIGHT_STATUS, { 0x01 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK(msg.State() == LightStates::On);
	BOOST_CHECK_EQUAL(msg.LightMode(), 0x00);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_UnknownState)
{
	LightMessage_Status msg;
	auto pkt = MakePacket(0xF0, CMD_LIGHT_STATUS, { 0x99, 0x02 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK(msg.State() == LightStates::Unknown);
	BOOST_CHECK_EQUAL(msg.LightMode(), 0x02);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_TooShort_NoState)
{
	LightMessage_Status msg;
	// size <= Index_State(4) → header only.
	std::vector<uint8_t> data = { 0x10, 0x02, 0xF0, CMD_LIGHT_STATUS };
	auto result = msg.DeserializeContents(std::span<const uint8_t>(data));
	BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(TestSerialize_WritesStateAndMode)
{
	LightMessage_Status msg;
	auto pkt = MakePacket(0xF0, CMD_LIGHT_STATUS, { 0x01, 0x07 });
	msg.DeserializeContents(std::span<const uint8_t>(pkt));

	std::vector<uint8_t> bytes;
	BOOST_CHECK(msg.SerializeContents(bytes));
	BOOST_REQUIRE_EQUAL(bytes.size(), 2u);
	BOOST_CHECK_EQUAL(bytes[0], 0x01); // On
	BOOST_CHECK_EQUAL(bytes[1], 0x07); // Mode
}

BOOST_AUTO_TEST_CASE(TestToString_ContainsStateAndMode)
{
	LightMessage_Status msg;
	auto pkt = MakePacket(0xF0, CMD_LIGHT_STATUS, { 0x01, 0x07 });
	msg.DeserializeContents(std::span<const uint8_t>(pkt));

	auto str = msg.ToString();
	BOOST_CHECK(str.find("On") != std::string::npos);
	BOOST_CHECK(str.find("7") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
