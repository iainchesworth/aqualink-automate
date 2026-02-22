#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/messages/heater/heater_message_request.h"
#include "jandy/messages/heater/heater_message_status.h"

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
// HeaterMessage_Request
// =============================================================================

BOOST_AUTO_TEST_SUITE(HeaterMessage_Request_TestSuite)

BOOST_AUTO_TEST_CASE(TestConstruction_Defaults)
{
	HeaterMessage_Request msg;
	BOOST_CHECK(msg.OperatingMode() == HeaterOperatingModes::Unknown);
	BOOST_CHECK_EQUAL(msg.PoolSetpoint(), 0);
	BOOST_CHECK_EQUAL(msg.SpaSetpoint(), 0);
	BOOST_CHECK_EQUAL(msg.WaterTemperature(), 0);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_ValidRequest)
{
	HeaterMessage_Request msg;
	// Index_OperatingMode=4, Index_PoolSetpoint=5, Index_SpaSetpoint=6, Index_WaterTemperature=7
	// HeatingPool = 0x19, pool_sp=82, spa_sp=100, water_temp=78
	auto pkt = MakePacket(0x60, 0x20, { 0x19, 82, 100, 78 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK(msg.OperatingMode() == HeaterOperatingModes::HeatingPool);
	BOOST_CHECK_EQUAL(msg.PoolSetpoint(), 82);
	BOOST_CHECK_EQUAL(msg.SpaSetpoint(), 100);
	BOOST_CHECK_EQUAL(msg.WaterTemperature(), 78);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_HeatPumpModes)
{
	HeaterMessage_Request msg;
	auto pkt = MakePacket(0x60, 0x20, { 0x09, 85, 102, 80 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK(msg.OperatingMode() == HeaterOperatingModes::HeatPumpHeatingPool);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_UnknownMode)
{
	HeaterMessage_Request msg;
	auto pkt = MakePacket(0x60, 0x20, { 0xAA, 85, 102, 80 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK(msg.OperatingMode() == HeaterOperatingModes::Unknown);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_OffMode)
{
	HeaterMessage_Request msg;
	auto pkt = MakePacket(0x60, 0x20, { 0x00, 82, 100, 0 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK(msg.OperatingMode() == HeaterOperatingModes::Off);
	BOOST_CHECK_EQUAL(msg.WaterTemperature(), 0);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_TooShort_NoMode)
{
	HeaterMessage_Request msg;
	// size <= Index_OperatingMode(4) → size must be <=4
	std::vector<uint8_t> short_data = { 0x10, 0x02, 0x60, 0x20 };
	auto result = msg.DeserializeContents(std::span<const uint8_t>(short_data));
	BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_TooShort_NoPool)
{
	HeaterMessage_Request msg;
	// size <= Index_PoolSetpoint(5) → need exactly 5 bytes
	std::vector<uint8_t> data = { 0x10, 0x02, 0x60, 0x20, 0x19 };
	auto result = msg.DeserializeContents(std::span<const uint8_t>(data));
	BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_TooShort_NoSpa)
{
	HeaterMessage_Request msg;
	// size <= Index_SpaSetpoint(6) → need exactly 6 bytes
	std::vector<uint8_t> data = { 0x10, 0x02, 0x60, 0x20, 0x19, 82 };
	auto result = msg.DeserializeContents(std::span<const uint8_t>(data));
	BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_TooShort_NoWaterTemp)
{
	HeaterMessage_Request msg;
	// size <= Index_WaterTemperature(7) → need exactly 7 bytes
	std::vector<uint8_t> data = { 0x10, 0x02, 0x60, 0x20, 0x19, 82, 100 };
	auto result = msg.DeserializeContents(std::span<const uint8_t>(data));
	BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(TestSerialize_WritesAllFields)
{
	HeaterMessage_Request msg;
	auto pkt = MakePacket(0x60, 0x20, { 0x19, 82, 100, 78 });
	msg.DeserializeContents(std::span<const uint8_t>(pkt));

	std::vector<uint8_t> bytes;
	BOOST_CHECK(msg.SerializeContents(bytes));
	BOOST_REQUIRE_EQUAL(bytes.size(), 4u);
	BOOST_CHECK_EQUAL(bytes[0], 0x19); // HeatingPool
	BOOST_CHECK_EQUAL(bytes[1], 82);   // PoolSetpoint
	BOOST_CHECK_EQUAL(bytes[2], 100);  // SpaSetpoint
	BOOST_CHECK_EQUAL(bytes[3], 78);   // WaterTemperature
}

BOOST_AUTO_TEST_CASE(TestToString_ContainsFields)
{
	HeaterMessage_Request msg;
	auto pkt = MakePacket(0x60, 0x20, { 0x19, 82, 100, 78 });
	msg.DeserializeContents(std::span<const uint8_t>(pkt));

	auto str = msg.ToString();
	BOOST_CHECK(str.find("HeatingPool") != std::string::npos);
	BOOST_CHECK(str.find("82") != std::string::npos);
	BOOST_CHECK(str.find("100") != std::string::npos);
	BOOST_CHECK(str.find("78") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// HeaterMessage_Status
// =============================================================================

BOOST_AUTO_TEST_SUITE(HeaterMessage_Status_TestSuite)

BOOST_AUTO_TEST_CASE(TestConstruction_Defaults)
{
	HeaterMessage_Status msg;
	BOOST_CHECK(msg.HeaterState() == HeaterStates::Unknown);
	BOOST_CHECK(msg.ErrorCode() == HeaterErrors::Unknown);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_HeatingNoError)
{
	HeaterMessage_Status msg;
	// Index_HeaterState=4, Index_ErrorCode=6 (byte[5] is reserved)
	// HeaterStates::Heating = 0x08, HeaterErrors::None = 0x00
	auto pkt = MakePacket(0x00, 0x21, { 0x08, 0x00, 0x00 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK(msg.HeaterState() == HeaterStates::Heating);
	BOOST_CHECK(msg.ErrorCode() == HeaterErrors::None);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_HeatPumpCooling_SensorFault)
{
	HeaterMessage_Status msg;
	auto pkt = MakePacket(0x00, 0x21, { 0x68, 0x00, 0x02 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK(msg.HeaterState() == HeaterStates::HeatPumpCooling);
	BOOST_CHECK(msg.ErrorCode() == HeaterErrors::SensorFault);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_Off_HighLimit)
{
	HeaterMessage_Status msg;
	auto pkt = MakePacket(0x00, 0x21, { 0x00, 0x00, 0x10 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK(msg.HeaterState() == HeaterStates::Off);
	BOOST_CHECK(msg.ErrorCode() == HeaterErrors::HighLimit);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_UnknownEnumValues)
{
	HeaterMessage_Status msg;
	auto pkt = MakePacket(0x00, 0x21, { 0x99, 0x00, 0x99 });
	auto result = msg.DeserializeContents(std::span<const uint8_t>(pkt));

	BOOST_CHECK(result);
	BOOST_CHECK(msg.HeaterState() == HeaterStates::Unknown);
	BOOST_CHECK(msg.ErrorCode() == HeaterErrors::Unknown);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_TooShort_NoState)
{
	HeaterMessage_Status msg;
	std::vector<uint8_t> data = { 0x10, 0x02, 0x00, 0x21 };
	auto result = msg.DeserializeContents(std::span<const uint8_t>(data));
	BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(TestDeserialize_TooShort_NoError)
{
	HeaterMessage_Status msg;
	// size <= Index_ErrorCode(6) → need <=6 bytes. Provide 6 bytes.
	std::vector<uint8_t> data = { 0x10, 0x02, 0x00, 0x21, 0x08, 0x00 };
	auto result = msg.DeserializeContents(std::span<const uint8_t>(data));
	BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(TestSerialize_WritesAllFields)
{
	HeaterMessage_Status msg;
	auto pkt = MakePacket(0x00, 0x21, { 0x08, 0x00, 0x00 });
	msg.DeserializeContents(std::span<const uint8_t>(pkt));

	std::vector<uint8_t> bytes;
	BOOST_CHECK(msg.SerializeContents(bytes));
	BOOST_REQUIRE_EQUAL(bytes.size(), 3u);
	BOOST_CHECK_EQUAL(bytes[0], 0x08); // Heating
	BOOST_CHECK_EQUAL(bytes[1], 0x00); // Reserved
	BOOST_CHECK_EQUAL(bytes[2], 0x00); // None
}

BOOST_AUTO_TEST_CASE(TestToString_ContainsFields)
{
	HeaterMessage_Status msg;
	auto pkt = MakePacket(0x00, 0x21, { 0x08, 0x00, 0x00 });
	msg.DeserializeContents(std::span<const uint8_t>(pkt));

	auto str = msg.ToString();
	BOOST_CHECK(str.find("Heating") != std::string::npos);
	BOOST_CHECK(str.find("None") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
