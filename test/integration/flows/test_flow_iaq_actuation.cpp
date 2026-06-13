#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <boost/asio/error.hpp>
#include <boost/test/unit_test.hpp>

#include "interfaces/icommanddispatcher.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/statistics_hub.h"
#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "protocol/message_generator_registry.h"
#include "protocol/protocol_thread.h"
#include "serial/serial_port.h"

#include "jandy/protocol/jandy_protocol_registration.h"
#include "jandy/devices/command_dispatcher.h"
#include "jandy/devices/iaq_device.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/iaq/iaq_message_control_data_response.h"

#include "mocks/mock_testserialportimpl.h"
#include "support/unit_test_hublocatorinjector.h"
#include "support/unit_test_loadfixture.h"
#include "support/unit_test_mockreplayharness.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Interfaces;
using namespace AqualinkAutomate::Kernel;
using namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes;

//=============================================================================
// IAQ (AqualinkTouch 0x33) ACTUATION -> THE WIRE, end-to-end.
//
// Verifies the capability-routed dispatcher drives an emulated AqualinkTouch to the
// exact RS-485 bytes a real iAqualink2 emits, decoded from the hardware capture
// test/fixtures/iaq_aux_setpoint.cap:
//   * aux on/off  = press the on-screen PageButton matching the label (0x11 + index):
//                   Pool Light is index 9 -> 0x1a, Spillway is index 11 -> 0x1c;
//   * setpoint    = value-submit: BACK(0x02) -> open Set Temp(0x14) -> select field
//                   (Pool Heat idx0 0x11 / Spa Heat idx2 0x13) -> submit(0x80), then on
//                   IAQ_ControlReady a control-data response "1<value>" (e.g. "131").
//
// No Serial Adapter is registered, so the IAQ (ranked Lowest) is the only DeviceActuator
// / SetpointController present and is selected. Button indices are learned from replayed
// PageButton frames exactly as they arrive on a live bus.
//=============================================================================

namespace
{
	constexpr uint8_t IAQ_ID = 0x33;

	constexpr uint8_t DLE = 0x10;
	constexpr uint8_t STX = 0x02;
	constexpr uint8_t ETX = 0x03;

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

	// A master->device PageButton (0x24) frame: payload = [index, status, 0x00, type, name...].
	// (Frame indices: ButtonIndex@4, ButtonState@5, ButtonType@7, NameText@8 - see
	// IAQMessage_PageButton.)
	std::vector<uint8_t> MakePageButton(uint8_t index, uint8_t status, const std::string& name)
	{
		std::vector<uint8_t> payload = { index, status, 0x00, 0x01 };
		for (char c : name) { payload.push_back(static_cast<uint8_t>(c)); }
		return MakeFramedPacket(IAQ_ID, static_cast<uint8_t>(Messages::JandyMessageIds::IAQ_PageButton), payload);
	}

	std::vector<uint8_t> ExpectedAckWireBytes(uint8_t ack_type, uint8_t command)
	{
		Messages::JandyMessage_Ack ack(ack_type, command);
		std::vector<uint8_t> bytes;
		ack.Serialize(bytes);
		return bytes;
	}

	std::vector<uint8_t> ExpectedControlDataWireBytes(const std::string& ascii)
	{
		Messages::IAQMessage_ControlDataResponse msg(ascii);
		std::vector<uint8_t> bytes;
		msg.Serialize(bytes);
		return bytes;
	}

	struct IAQActuationFixture : public AqualinkAutomate::Test::HubLocatorInjector
	{
		IAQActuationFixture()
			: data_hub(Find<DataHub>())
			, equipment_hub(Find<EquipmentHub>())
			, statistics_hub(Find<StatisticsHub>())
			, serial_impl(new Test::TestSerialPortImpl())
			, serial_port(std::make_shared<Serial::SerialPort>(std::unique_ptr<Test::TestSerialPortImpl>(serial_impl), *this))
			, dispatcher(std::make_shared<CommandDispatcher>(data_hub, equipment_hub))
		{
			serial_impl->EnableTestMode(true);
			Register<Interfaces::ICommandDispatcher>(dispatcher);

			Protocol::MessageGeneratorRegistry::Instance().Clear();
			Jandy::Protocol::RegisterMessageGenerator();

			protocol_task = std::make_shared<Protocol::ProtocolTask>(serial_port, statistics_hub);
			protocol_task->ConnectWriteSignal<Messages::JandyMessage_Ack>();
			protocol_task->ConnectWriteSignal<Messages::IAQMessage_ControlDataResponse>();

			// ONLY an emulated IAQ -- no Serial Adapter -- so the IAQ (Lowest) is the chosen
			// DeviceActuator / SetpointController.
			auto iaq_id = std::make_shared<JandyDeviceType>(JandyDeviceId(IAQ_ID));
			equipment_hub->AddDevice(std::make_unique<IAQDevice>(iaq_id, *this, true));
		}

		~IAQActuationFixture() override
		{
			protocol_task.reset();
			Protocol::MessageGeneratorRegistry::Instance().Clear();
			if (serial_impl != nullptr) { serial_impl->Reset(); }
		}

		void Replay(const std::vector<uint8_t>& frame)
		{
			serial_impl->QueueReadData(frame);
			serial_impl->QueueReadData({}, boost::asio::error::would_block);
			(void)protocol_task->Poll();
		}

		void ReplayIAQPoll() { Replay(MakeFramedPacket(IAQ_ID, static_cast<uint8_t>(Messages::JandyMessageIds::IAQ_Poll))); }
		void ReplayIAQControlReady() { Replay(MakeFramedPacket(IAQ_ID, static_cast<uint8_t>(Messages::JandyMessageIds::IAQ_ControlReady))); }

		// Establish the home-page button table the device resolves labels against.
		void SeedHomePageButtons()
		{
			Replay(MakePageButton(9, 0x01, "Pool LightON"));    // index 9, ON
			Replay(MakePageButton(11, 0x01, "SpillwayON"));     // index 11, ON
		}

		const std::vector<uint8_t>& Wire() const { return serial_impl->GetWrittenData(); }
		void ClearWire() { serial_impl->ClearWrittenData(); }

		std::shared_ptr<DataHub> data_hub;
		std::shared_ptr<EquipmentHub> equipment_hub;
		std::shared_ptr<StatisticsHub> statistics_hub;
		Test::TestSerialPortImpl* serial_impl;
		std::shared_ptr<Serial::SerialPort> serial_port;
		std::shared_ptr<Protocol::ProtocolTask> protocol_task;
		std::shared_ptr<CommandDispatcher> dispatcher;
	};

	std::shared_ptr<AuxillaryDevice> AddAux(DataHub& data_hub, const std::string& label)
	{
		auto device = std::make_shared<AuxillaryDevice>();
		device->AuxillaryTraits.Set(LabelTrait{}, label);
		device->AuxillaryTraits.Set(AuxillaryTypeTrait{}, AuxillaryTypes::Auxillary);
		data_hub.Devices.Add(device);
		return device;
	}
}

BOOST_FIXTURE_TEST_SUITE(TestSuite_Integration_Flow_IAQActuation, IAQActuationFixture)

//-----------------------------------------------------------------------------
// Aux toggle: the label is mapped to the live PageButton index and pressed as
// 0x11 + index on the next IAQ_Poll.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(ToggleByLabel_PoolLight_WritesPageButtonPressToWire)
{
	SeedHomePageButtons();
	AddAux(*data_hub, "Pool Light");

	auto result = dispatcher->ToggleByLabel("Pool Light");
	BOOST_REQUIRE_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::Success));

	ClearWire();
	ReplayIAQPoll();
	// Pool Light = button index 9 -> 0x11 + 9 = 0x1a (verified vs hardware).
	BOOST_CHECK(Wire() == ExpectedAckWireBytes(0x00, 0x1a));
}

BOOST_AUTO_TEST_CASE(ToggleByLabel_Spillway_WritesPageButtonPressToWire)
{
	SeedHomePageButtons();
	AddAux(*data_hub, "Spillway");

	auto result = dispatcher->ToggleByLabel("Spillway");
	BOOST_REQUIRE_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::Success));

	ClearWire();
	ReplayIAQPoll();
	// Spillway = button index 11 -> 0x11 + 11 = 0x1c (verified vs hardware).
	BOOST_CHECK(Wire() == ExpectedAckWireBytes(0x00, 0x1c));
}

BOOST_AUTO_TEST_CASE(ToggleByLabel_UnknownButton_DoesNotReachWire)
{
	SeedHomePageButtons();
	AddAux(*data_hub, "Nonexistent Device");

	// Present on the bus, but no on-screen button matches -> mapping failed (not Success).
	auto result = dispatcher->ToggleByLabel("Nonexistent Device");
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::UnknownEquipmentType));
}

//-----------------------------------------------------------------------------
// PRECEDENCE: with BOTH an AqualinkTouch (Medium) and a OneTouch (Low) present,
// the dispatcher must select the AqualinkTouch (direct page-button) over the
// slower OneTouch menu-nav. Proven by the IAQ emitting its page-button press on
// the wire; had the OneTouch been chosen, the IAQ would have nothing to send.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(ToggleByLabel_PrefersAqualinkTouchOverOneTouch)
{
	SeedHomePageButtons();
	AddAux(*data_hub, "Pool Light");

	// Add an emulated OneTouch alongside the IAQ (a combined rig).
	auto ot_id = std::make_shared<JandyDeviceType>(JandyDeviceId(0x40));
	equipment_hub->AddDevice(std::make_unique<OneTouchDevice>(ot_id, *this, true));

	auto result = dispatcher->ToggleByLabel("Pool Light");
	BOOST_REQUIRE_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::Success));

	ClearWire();
	ReplayIAQPoll();
	// The IAQ (Medium) was chosen, so it presses Pool Light (index 9 -> 0x1a).
	BOOST_CHECK(Wire() == ExpectedAckWireBytes(0x00, 0x1a));
}

//-----------------------------------------------------------------------------
// Setpoint: the value-submit nav sequence, then a control-data value on
// IAQ_ControlReady.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(SetPoolSetpoint_WritesNavSequenceThenControlDataToWire)
{
	auto result = dispatcher->SetPoolSetpoint(31);
	BOOST_REQUIRE_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::Success));

	// Phase 1: nav ACKs {BACK, open Set Temp, select Pool Heat, submit}, one per poll.
	const std::vector<uint8_t> expected = { 0x02, 0x14, 0x11, 0x80 };
	for (auto cmd : expected)
	{
		ClearWire();
		ReplayIAQPoll();
		BOOST_CHECK(Wire() == ExpectedAckWireBytes(0x00, cmd));
	}

	// Phase 2: control-data response "1" + value = "131" (pool 31).
	ClearWire();
	ReplayIAQControlReady();
	BOOST_CHECK(Wire() == ExpectedControlDataWireBytes("131"));
}

BOOST_AUTO_TEST_CASE(SetSpaSetpoint_WritesNavSequenceThenControlDataToWire)
{
	auto result = dispatcher->SetSpaSetpoint(39);
	BOOST_REQUIRE_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::Success));

	// Spa Heat is button index 2 -> 0x13 (instead of Pool Heat's 0x11).
	const std::vector<uint8_t> expected = { 0x02, 0x14, 0x13, 0x80 };
	for (auto cmd : expected)
	{
		ClearWire();
		ReplayIAQPoll();
		BOOST_CHECK(Wire() == ExpectedAckWireBytes(0x00, cmd));
	}

	ClearWire();
	ReplayIAQControlReady();
	BOOST_CHECK(Wire() == ExpectedControlDataWireBytes("139"));
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// Provenance: the hardware capture the byte sequences above were decoded from
// (test/fixtures/iaq_aux_setpoint.cap: Pool Light + Spillway toggled, pool/spa
// setpoints edited) round-trips cleanly through the full Jandy decode stack.
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_Integration_Flow_IAQActuationCapture)

BOOST_AUTO_TEST_CASE(Replay_IAQAuxSetpointSession_DecodesCleanly)
{
	AqualinkAutomate::Test::MockReplayHarness harness;

	auto device_id = std::make_shared<JandyDeviceType>(JandyDeviceId(IAQ_ID));
	auto iaq = std::make_shared<IAQDevice>(device_id, harness.HubLocatorRef(), false);

	auto replayed_bytes = AqualinkAutomate::Test::ReplayFixture(harness, "fixtures/iaq_aux_setpoint.cap");

	BOOST_REQUIRE(!replayed_bytes.empty());
	BOOST_CHECK_GT(replayed_bytes.size(), 1000u);
	BOOST_CHECK_EQUAL(harness.StatisticsHub()->MessageErrors.ChecksumFailures, 0u);

	// A passive IAQ enters NormalOperation on its first update, proving the captured
	// AqualinkTouch traffic drove the device.
	BOOST_CHECK(iaq->IsInNormalOperation());

	iaq.reset();
}

BOOST_AUTO_TEST_SUITE_END()
