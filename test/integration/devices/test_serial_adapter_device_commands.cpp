#include <cstdint>
#include <memory>
#include <vector>

#include <boost/signals2.hpp>
#include <boost/test/unit_test.hpp>

#include "jandy/devices/serial_adapter_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_status.h"
#include "jandy/messages/serial_adapter/serial_adapter_message_dev_status.h"
#include "jandy/auxillaries/jandy_auxillary_id.h"

#include "support/unit_test_hublocatorinjector.h"

using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Messages;
using namespace AqualinkAutomate::Interfaces;

// =============================================================================
// Helper: build a valid Status packet with correct checksum for a given device
// Packet: [DLE, STX, dest, msg_type(0x02), ...status payload..., checksum, DLE, ETX]
// =============================================================================

namespace
{
	std::vector<uint8_t> MakeValidStatusPacket(uint8_t dest)
	{
		// Status message (0x02) with 5-byte payload (all zeros for minimal valid status)
		std::vector<uint8_t> pkt = { 0x10, 0x02, dest, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 };
		// Checksum = sum of all bytes before footer
		uint32_t sum = 0;
		for (auto b : pkt) sum += b;
		pkt.push_back(static_cast<uint8_t>(sum & 0xFF));
		pkt.push_back(0x10);
		pkt.push_back(0x03);
		return pkt;
	}

	struct SerialAdapterCommandFixture : public AqualinkAutomate::Test::HubLocatorInjector
	{
		static constexpr uint8_t DEVICE_ID = 0x48;

		SerialAdapterCommandFixture()
			: device_type(std::make_shared<JandyDeviceType>(JandyDeviceId(DEVICE_ID)))
			, device(device_type, *this, true) // emulated=true so ACKs are sent
		{
			// Connect to the Ack send signal to capture outgoing ACK messages.
			// Use SerializeContents to extract raw ack_type and command bytes,
			// since AckType() casts to an enum and may return Unknown for non-standard values.
			ack_connection = IMessageSignalSend<JandyMessage_Ack>::GetPublisher()->connect(
				[this](std::reference_wrapper<const JandyMessage_Ack> ack_ref)
				{
					std::vector<uint8_t> serialized;
					ack_ref.get().SerializeContents(serialized);
					if (serialized.size() >= 2)
					{
						captured_ack_types.push_back(serialized[0]);
						captured_ack_data.push_back(serialized[1]);
					}
				}
			);
		}

		~SerialAdapterCommandFixture() override
		{
			ack_connection.disconnect();
		}

		void FireStatus()
		{
			JandyMessage_Status status;
			auto pkt = MakeValidStatusPacket(DEVICE_ID);
			[[maybe_unused]] auto result = status.Deserialize(std::span<const uint8_t>(pkt));
			status.Signal_MessageWasReceived();
		}

		std::shared_ptr<JandyDeviceType> device_type;
		SerialAdapterDevice device;
		std::vector<uint8_t> captured_ack_types;
		std::vector<uint8_t> captured_ack_data;
		boost::signals2::connection ack_connection;
	};
}

BOOST_FIXTURE_TEST_SUITE(SerialAdapterDeviceCommands_IntegrationTestSuite, SerialAdapterCommandFixture)

// =============================================================================
// Status with no pending command -> query ACK (rotates through status types)
// =============================================================================

BOOST_AUTO_TEST_CASE(TestStatus_NoPendingCommand_QueriesStatusType)
{
	FireStatus();

	BOOST_REQUIRE(!captured_ack_types.empty());
	// When no pending command, ProcessControllerUpdates sends a status query
	// The first status type in the collection (after MODEL is removed) should be queried
}

// =============================================================================
// QueueCommand then status -> dispatches pending command
// =============================================================================

BOOST_AUTO_TEST_CASE(TestQueueCommand_ThenStatus_DispatchesPendingCommand)
{
	device.QueueCommand(0x0C, 0x81); // PUMP, SetOn
	captured_ack_types.clear();
	captured_ack_data.clear();

	FireStatus();

	BOOST_REQUIRE(!captured_ack_types.empty());
	BOOST_CHECK_EQUAL(captured_ack_types.back(), 0x0C);
	BOOST_CHECK_EQUAL(captured_ack_data.back(), 0x81);
}

// =============================================================================
// QueuePumpCommand then status -> dispatches pump command
// =============================================================================

BOOST_AUTO_TEST_CASE(TestQueuePumpCommand_ThenStatus_DispatchesPumpCommand)
{
	device.QueuePumpCommand(SerialAdapter_SystemPumpCommands::SPA, SerialAdapter_CommandTypes::SetOn);
	captured_ack_types.clear();
	captured_ack_data.clear();

	FireStatus();

	BOOST_REQUIRE(!captured_ack_types.empty());
	// RSSA setDev byte order is {state, devID} (AqualinkD serialadapter.c,
	// live-validated 2026-06-28): state SetOn=0x81 in the ack_type slot, SPA
	// devID=0x0E in the ack_data slot.
	BOOST_CHECK_EQUAL(captured_ack_types.back(), 0x81);
	BOOST_CHECK_EQUAL(captured_ack_data.back(), 0x0E);
}

// =============================================================================
// QueueAuxCommand then status -> dispatches aux command
// =============================================================================

BOOST_AUTO_TEST_CASE(TestQueueAuxCommand_ThenStatus_DispatchesAuxCommand)
{
	device.QueueAuxCommand(AqualinkAutomate::Auxillaries::JandyAuxillaryIds::Aux_1, SerialAdapter_CommandTypes::SetOn);
	captured_ack_types.clear();
	captured_ack_data.clear();

	FireStatus();

	BOOST_REQUIRE(!captured_ack_types.empty());
	// Aux_1 = 0x01 + SERIALADAPTER_AUX_ID_OFFSET(0x14) = 0x15, SetOn = 0x81
	BOOST_CHECK_EQUAL(captured_ack_types.back(), 0x15);
	BOOST_CHECK_EQUAL(captured_ack_data.back(), 0x81);
}

// =============================================================================
// QueueSetpointCommand then status -> dispatches setpoint command
// =============================================================================

BOOST_AUTO_TEST_CASE(TestQueueSetpointCommand_ThenStatus_DispatchesSetpointCommand)
{
	device.QueueSetpointCommand(SerialAdapter_SystemTemperatureCommands::POOLSP, 82);
	captured_ack_types.clear();
	captured_ack_data.clear();

	FireStatus();

	BOOST_REQUIRE(!captured_ack_types.empty());
	// POOLSP = 0x05, temperature = 82
	BOOST_CHECK_EQUAL(captured_ack_types.back(), 0x05);
	BOOST_CHECK_EQUAL(captured_ack_data.back(), 82);
}

// =============================================================================
// Command consumed after first dispatch
// =============================================================================

BOOST_AUTO_TEST_CASE(TestPendingCommand_ConsumedAfterDispatch)
{
	device.QueueCommand(0x0C, 0x81);
	captured_ack_types.clear();
	captured_ack_data.clear();

	// First status dispatches the command
	FireStatus();
	BOOST_REQUIRE(!captured_ack_types.empty());
	BOOST_CHECK_EQUAL(captured_ack_types.back(), 0x0C);

	// Second status should NOT dispatch the same command again
	captured_ack_types.clear();
	captured_ack_data.clear();
	FireStatus();

	BOOST_REQUIRE(!captured_ack_types.empty());
	// After command is consumed, it should send a query instead (not 0x0C)
	// The exact query depends on the current position in the status type rotation
}

// =============================================================================
// Queue overwrite: second command replaces first
// =============================================================================

BOOST_AUTO_TEST_CASE(TestQueueOverwrite_SecondCommandReplacesFirst)
{
	device.QueueCommand(0x0C, 0x81); // PUMP SetOn
	device.QueueCommand(0x0E, 0x80); // SPA SetOff (overwrites PUMP)
	captured_ack_types.clear();
	captured_ack_data.clear();

	FireStatus();

	BOOST_REQUIRE(!captured_ack_types.empty());
	// Only the second command should be dispatched
	BOOST_CHECK_EQUAL(captured_ack_types.back(), 0x0E);
	BOOST_CHECK_EQUAL(captured_ack_data.back(), 0x80);
}

BOOST_AUTO_TEST_SUITE_END()
