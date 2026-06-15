#include <cstdint>
#include <memory>
#include <vector>

#include <boost/asio/error.hpp>
#include <boost/test/unit_test.hpp>

#include "kernel/equipment_hub.h"
#include "kernel/statistics_hub.h"
#include "protocol/message_generator_registry.h"
#include "protocol/protocol_thread.h"
#include "serial/serial_port.h"

#include "jandy/protocol/jandy_protocol_registration.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/devices/spaside_remote_device.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_ids.h"

#include "mocks/mock_testserialportimpl.h"
#include "support/unit_test_hublocatorinjector.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Kernel;

//=============================================================================
// Spa-side remote EMULATION write-path test: PressButton() -> SpasideRemoteDevice
// -> Protocol::ProtocolTask -> Serial::SerialPort -> THE WIRE.
//
// Mirrors test/integration/flows/test_flow_command_to_wire.cpp: it asserts on the
// raw bytes actually WRITTEN to the serial port (Test::TestSerialPortImpl write
// capture).  The master polls the emulated remote with a cmd-0x02 LED-image frame
// (which decodes as a JandyMessage_Status addressed to our id); the remote replies
// with [0x01][0x00][button] -- a JandyMessage_Ack, ack_type 0x00, command = the
// pressed-button index (0 = idle).  PressButton(N) injects N into the next reply
// and is momentary (released to 0 after a single report).
//
// GENERALITY (open source): this deliberately emulates a "Spa Link" at 0x20
// (DeviceClasses::SpaRemote) -- the spaside class the maintainer does NOT own --
// so the write path is proven for a configuration other than the maintainer's own
// Dual Spa Switch (0x10).  The maintainer's real 0x10 captures cover the read path
// as fixtures; this synthetic 0x20 emulation covers the write path generically.
//=============================================================================

namespace
{
	constexpr uint8_t SPA_LINK_ID = 0x20;	// DeviceClasses::SpaRemote, instance 0

	constexpr uint8_t DLE = 0x10;
	constexpr uint8_t STX = 0x02;
	constexpr uint8_t ETX = 0x03;

	// Build a fully-framed+checksummed Jandy packet so it passes the real
	// generator's validation when replayed.
	std::vector<uint8_t> MakeFramedPacket(uint8_t dest, uint8_t msg_type, const std::vector<uint8_t>& payload = {})
	{
		std::vector<uint8_t> pkt = { DLE, STX, dest, msg_type };
		pkt.insert(pkt.end(), payload.begin(), payload.end());
		uint32_t sum = 0;
		for (auto b : pkt) { sum += b; }
		pkt.push_back(static_cast<uint8_t>(sum & 0xFF));
		pkt.push_back(DLE);
		pkt.push_back(ETX);
		return pkt;
	}

	// The exact bytes ProtocolTask writes for a given ACK, computed by the SAME
	// production serializer the write path uses.
	std::vector<uint8_t> ExpectedAckWireBytes(uint8_t ack_type, uint8_t command)
	{
		Messages::JandyMessage_Ack ack(ack_type, command);
		std::vector<uint8_t> bytes;
		ack.Serialize(bytes);
		return bytes;
	}

	struct SpasideRemoteToWireFixture : public AqualinkAutomate::Test::HubLocatorInjector
	{
		SpasideRemoteToWireFixture()
			: equipment_hub(Find<EquipmentHub>())
			, statistics_hub(Find<StatisticsHub>())
			, serial_impl(new Test::TestSerialPortImpl())
			, serial_port(std::make_shared<Serial::SerialPort>(
				std::unique_ptr<Test::TestSerialPortImpl>(serial_impl), *this))
		{
			serial_impl->EnableTestMode(true);

			// Register ONLY the Jandy generator (isolated from any leftover).
			Protocol::MessageGeneratorRegistry::Instance().Clear();
			Jandy::Protocol::RegisterMessageGenerator();

			protocol_task = std::make_shared<Protocol::ProtocolTask>(serial_port, statistics_hub);

			// Connect the ACK write signal exactly as aqualink-automate.cpp does, so the
			// remote's emitted [0x01][0x00][button] reply is serialized and written.
			protocol_task->ConnectWriteSignal<Messages::JandyMessage_Ack>();

			// Emulated Spa Link at 0x20 (is_emulated=true so it actively replies). Keep a
			// raw pointer so the test can drive PressButton(); ownership is the hub's.
			auto id = std::make_shared<JandyDeviceType>(JandyDeviceId(SPA_LINK_ID));
			auto device = std::make_unique<SpasideRemoteDevice>(id, *this, true);
			remote = device.get();
			equipment_hub->AddDevice(std::move(device));
		}

		~SpasideRemoteToWireFixture() override
		{
			protocol_task.reset();
			Protocol::MessageGeneratorRegistry::Instance().Clear();
			if (serial_impl != nullptr)
			{
				serial_impl->Reset();
			}
		}

		// Replay the master's cmd-0x02 LED-image poll to our address (6-byte image,
		// mirroring the real on-wire frame).  Decodes as a Status to 0x20.
		void ReplayMasterPoll()
		{
			const auto frame = MakeFramedPacket(SPA_LINK_ID,
				static_cast<uint8_t>(Messages::JandyMessageIds::Status),
				{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
			serial_impl->QueueReadData(frame);
			serial_impl->QueueReadData({}, boost::asio::error::would_block);
			(void)protocol_task->Poll();
		}

		const std::vector<uint8_t>& Wire() const { return serial_impl->GetWrittenData(); }
		void ClearWire() { serial_impl->ClearWrittenData(); }

		std::shared_ptr<EquipmentHub> equipment_hub;
		std::shared_ptr<StatisticsHub> statistics_hub;

		Test::TestSerialPortImpl* serial_impl;	// owned by serial_port
		std::shared_ptr<Serial::SerialPort> serial_port;
		std::shared_ptr<Protocol::ProtocolTask> protocol_task;
		SpasideRemoteDevice* remote;			// owned by equipment_hub
	};
}

BOOST_FIXTURE_TEST_SUITE(TestSuite_Integration_Flow_SpasideRemote, SpasideRemoteToWireFixture)

//-----------------------------------------------------------------------------
// Sanity: with NO button queued, a master poll still draws an IDLE reply
// [0x01][0x00][0x00] -- a present remote answers every poll with button 0.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(MasterPoll_NoButton_WritesIdleAck)
{
	ClearWire();
	ReplayMasterPoll();

	BOOST_CHECK(Wire() == ExpectedAckWireBytes(0x00, 0x00));
}

//-----------------------------------------------------------------------------
// PressButton(2) injects button index 2 into the next reply: the wire carries
// [0x01][0x00][0x02] so the master actuates whatever button 2 is mapped to on
// the controller.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(PressButton_InjectsButtonIntoNextAck)
{
	remote->PressButton(2);

	ClearWire();
	ReplayMasterPoll();

	BOOST_CHECK(Wire() == ExpectedAckWireBytes(0x00, 0x02));
}

//-----------------------------------------------------------------------------
// A pressed button is MOMENTARY: it is reported on exactly one poll, then the
// remote releases back to idle (button 0) on the following poll -- mimicking a
// brief physical keypress rather than a latched switch.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(PressButton_IsMomentary_ReleasesAfterOneReport)
{
	remote->PressButton(3);

	ClearWire();
	ReplayMasterPoll();
	BOOST_CHECK(Wire() == ExpectedAckWireBytes(0x00, 0x03));

	// Next poll: released, so back to the idle reply.
	ClearWire();
	ReplayMasterPoll();
	BOOST_CHECK(Wire() == ExpectedAckWireBytes(0x00, 0x00));
}

BOOST_AUTO_TEST_SUITE_END()
