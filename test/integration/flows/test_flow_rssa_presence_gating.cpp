#include <cstdint>
#include <memory>
#include <vector>

#include <boost/asio/error.hpp>
#include <boost/test/unit_test.hpp>

#include <magic_enum/magic_enum.hpp>

#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/statistics_hub.h"
#include "kernel/temperature.h"
#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/auxillary_devices/auxillary_status.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "protocol/message_generator_registry.h"
#include "protocol/protocol_thread.h"
#include "serial/serial_port.h"

#include "jandy/protocol/jandy_protocol_registration.h"
#include "jandy/auxillaries/jandy_auxillary_id.h"
#include "jandy/auxillaries/jandy_auxillary_status.h"
#include "jandy/auxillaries/jandy_auxillary_traits_types.h"
#include "jandy/devices/serial_adapter_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/serial_adapter/serial_adapter_message_dev_status.h"

#include "mocks/mock_testserialportimpl.h"
#include "support/unit_test_hublocatorinjector.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Kernel;
using namespace AqualinkAutomate::Messages;

//=============================================================================
// PRESENCE-GATED, READ+WRITE RSSA EMULATION -- full-stack flow tests.
//
// These drive the production path end-to-end (TestSerialPortImpl -> SerialPort
// -> ProtocolTask -> Jandy generator -> message signals -> SerialAdapterDevice
// slots -> DataHub) and additionally capture the raw bytes the emulated device
// WRITES to the wire via Test::TestSerialPortImpl's write capture.
//
// Coverage:
//   (a) a master Status poll to the emulated 0x48 emits a query ACK to the wire;
//   (b) RSSA DevStatus (0x13) replies decode setpoints / units / aux into the
//       DataHub (the read path);
//   (c) PRESENCE GATING: after a real DevStatus is observed at 0x48 the emulated
//       instance stops transmitting (a subsequent poll writes nothing) -- never
//       two transmitters on the same bus address;
//   (d) the CAPTURE-GATED writes (two-step setpoint, aux toggle) round-trip to
//       the exact AqualinkD-derived wire bytes.
//
// DevStatus frames are built with destination 0x48 so they match the device's
// id-filtered slots, consistent with the existing RSSA read/decode path.
//=============================================================================

namespace
{
	constexpr uint8_t SERIAL_ADAPTER_ID = 0x48;

	constexpr uint8_t DLE = 0x10;
	constexpr uint8_t STX = 0x02;
	constexpr uint8_t ETX = 0x03;

	// Build a fully-framed + checksummed Jandy packet (same layout the generator
	// validates), so a frame produced here passes the real decode pipeline.
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
	// production serializer the write path uses. Comparing captured wire bytes
	// against this proves the command reached the port intact.
	std::vector<uint8_t> ExpectedAckWireBytes(uint8_t ack_type, uint8_t command)
	{
		JandyMessage_Ack ack(ack_type, command);
		std::vector<uint8_t> bytes;
		ack.Serialize(bytes);
		return bytes;
	}

	//-------------------------------------------------------------------------
	// Full-stack fixture: real hubs + serial port + protocol task, with an
	// EMULATED SerialAdapter registered at 0x48 and the JandyMessage_Ack write
	// signal connected exactly as production does.
	//-------------------------------------------------------------------------
	struct RssaPresenceFixture : public AqualinkAutomate::Test::HubLocatorInjector
	{
		RssaPresenceFixture()
			: data_hub(Find<DataHub>())
			, equipment_hub(Find<EquipmentHub>())
			, statistics_hub(Find<StatisticsHub>())
			, serial_impl(new Test::TestSerialPortImpl())
			, serial_port(std::make_shared<Serial::SerialPort>(
				std::unique_ptr<Test::TestSerialPortImpl>(serial_impl), *this))
		{
			serial_impl->EnableTestMode(true);

			Protocol::MessageGeneratorRegistry::Instance().Clear();
			Jandy::Protocol::RegisterMessageGenerator();

			protocol_task = std::make_shared<Protocol::ProtocolTask>(serial_port, statistics_hub);
			protocol_task->ConnectWriteSignal<JandyMessage_Ack>();

			auto sa_id = std::make_shared<JandyDeviceType>(JandyDeviceId(SERIAL_ADAPTER_ID));
			sa_device = std::make_shared<SerialAdapterDevice>(sa_id, *this, true /* emulated */);
		}

		~RssaPresenceFixture() override
		{
			sa_device.reset();
			protocol_task.reset();
			Protocol::MessageGeneratorRegistry::Instance().Clear();
			if (serial_impl != nullptr)
			{
				serial_impl->Reset();
			}
		}

		// Replay one framed packet through the full protocol task.
		void Replay(const std::vector<uint8_t>& frame)
		{
			serial_impl->QueueReadData(frame);
			serial_impl->QueueReadData({}, boost::asio::error::would_block);
			(void)protocol_task->Poll();
		}

		// A master Status poll addressed to 0x48 (drives the device to emit an ACK).
		void ReplaySerialAdapterStatus()
		{
			Replay(MakeFramedPacket(SERIAL_ADAPTER_ID, static_cast<uint8_t>(JandyMessageIds::Status), { 0x00, 0x00, 0x00, 0x00, 0x00 }));
		}

		// An RSSA DevStatus (0x13) reply addressed to 0x48 with the given 4-byte
		// payload occupying frame bytes [4..7] (statustype, b5, b6, devid).
		void ReplayDevStatus(uint8_t status_type, uint8_t b5, uint8_t b6, uint8_t b7)
		{
			Replay(MakeFramedPacket(SERIAL_ADAPTER_ID, static_cast<uint8_t>(JandyMessageIds::RSSA_DevStatus), { status_type, b5, b6, b7 }));
		}

		const std::vector<uint8_t>& Wire() const { return serial_impl->GetWrittenData(); }
		void ClearWire() { serial_impl->ClearWrittenData(); }

		std::shared_ptr<DataHub> data_hub;
		std::shared_ptr<EquipmentHub> equipment_hub;
		std::shared_ptr<StatisticsHub> statistics_hub;

		Test::TestSerialPortImpl* serial_impl;	// owned by serial_port
		std::shared_ptr<Serial::SerialPort> serial_port;
		std::shared_ptr<Protocol::ProtocolTask> protocol_task;
		std::shared_ptr<SerialAdapterDevice> sa_device;
	};
}

BOOST_FIXTURE_TEST_SUITE(TestSuite_Integration_Flow_RssaPresenceGating, RssaPresenceFixture)

//=============================================================================
// (a) Emulated device responds to the master's poll for 0x48.
//=============================================================================

BOOST_AUTO_TEST_CASE(EmulatedAdapter_RespondsToMasterStatusPoll)
{
	// Sanity: at construction nothing has been transmitted (it waits to be polled,
	// like the OneTouch emulation -- it never blasts ACKs before being polled).
	BOOST_CHECK(Wire().empty());

	ClearWire();
	ReplaySerialAdapterStatus();

	// A framed Jandy ACK packet (the query) must now be on the wire.
	BOOST_REQUIRE(!Wire().empty());
	BOOST_CHECK_GE(Wire().size(), 8u);
	BOOST_CHECK_EQUAL(Wire().front(), DLE);
	BOOST_CHECK_EQUAL(Wire().back(), ETX);
}

//=============================================================================
// (b) READ: DevStatus replies decode into the DataHub.
//=============================================================================

BOOST_AUTO_TEST_CASE(Read_Units_DecodeIntoDataHub)
{
	// UNITS (0x0A): byte[6] == 0x00 -> Fahrenheit; non-zero -> Celsius.
	ReplayDevStatus(static_cast<uint8_t>(SerialAdapter_SystemTemperatureCommands::UNITS), 0x00, 0x01, 0x00);
	BOOST_CHECK(data_hub->SystemTemperatureUnits() == TemperatureUnits::Celsius);

	ReplayDevStatus(static_cast<uint8_t>(SerialAdapter_SystemTemperatureCommands::UNITS), 0x00, 0x00, 0x00);
	BOOST_CHECK(data_hub->SystemTemperatureUnits() == TemperatureUnits::Fahrenheit);
}

BOOST_AUTO_TEST_CASE(Read_PoolAndSpaSetpoints_DecodeIntoDataHub)
{
	// Query UNITS first so setpoint raw values are interpreted in the right scale
	// (Fahrenheit here), exactly as the device sequences its real poll cycle.
	ReplayDevStatus(static_cast<uint8_t>(SerialAdapter_SystemTemperatureCommands::UNITS), 0x00, 0x00, 0x00);
	BOOST_REQUIRE(data_hub->SystemTemperatureUnits() == TemperatureUnits::Fahrenheit);

	// POOLSP (0x05): byte[6] carries the setpoint value (82F).
	ReplayDevStatus(static_cast<uint8_t>(SerialAdapter_SystemTemperatureCommands::POOLSP), 0x00, 82, 0x00);
	BOOST_REQUIRE(data_hub->PoolTempSetpoint().has_value());
	BOOST_CHECK_CLOSE(data_hub->PoolTempSetpoint().value().InFahrenheit().value(), 82.0, 0.5);

	// SPASP (0x07): byte[6] carries the spa setpoint value (104F).
	ReplayDevStatus(static_cast<uint8_t>(SerialAdapter_SystemTemperatureCommands::SPASP), 0x00, 104, 0x00);
	BOOST_REQUIRE(data_hub->SpaTempSetpoint().has_value());
	BOOST_CHECK_CLOSE(data_hub->SpaTempSetpoint().value().InFahrenheit().value(), 104.0, 0.5);
}

BOOST_AUTO_TEST_CASE(Read_AuxState_DecodeIntoDataHub)
{
	// A device-status reply uses statustype 0x03; byte[7] = aux devID (aux + 0x14),
	// byte[6] = JandyAuxillaryStatuses (On=0x01). Aux_1 -> devID 0x15.
	const uint8_t aux1_dev_id = static_cast<uint8_t>(magic_enum::enum_integer(Auxillaries::JandyAuxillaryIds::Aux_1) + SerialAdapterMessage_DevStatus::SERIALADAPTER_AUX_ID_OFFSET);
	ReplayDevStatus(0x03, 0x00, static_cast<uint8_t>(Auxillaries::JandyAuxillaryStatuses::On), aux1_dev_id);

	// The aux device should have been created in the DataHub with On status.
	auto auxillaries = data_hub->Devices.FindByTrait(Auxillaries::JandyAuxillaryId{});
	BOOST_REQUIRE(!auxillaries.empty());

	bool found_on_aux1 = false;
	for (const auto& aux : auxillaries)
	{
		if (aux == nullptr || !aux->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}))
		{
			continue;
		}
		if (*(aux->AuxillaryTraits[Auxillaries::JandyAuxillaryId{}]) == Auxillaries::JandyAuxillaryIds::Aux_1)
		{
			auto status_opt = aux->AuxillaryTraits.TryGet(AuxillaryTraitsTypes::AuxillaryStatusTrait{});
			BOOST_REQUIRE(status_opt.has_value());
			BOOST_CHECK(status_opt.value() == AuxillaryStatuses::On);
			found_on_aux1 = true;
		}
	}
	BOOST_CHECK(found_on_aux1);
}

//=============================================================================
// (c) PRESENCE GATING: once a real adapter is observed, the emulated instance
// must permanently stop transmitting (never two transmitters on 0x48).
//=============================================================================

BOOST_AUTO_TEST_CASE(PresenceGating_RealDevStatus_SilencesEmulatedAdapter)
{
	// Before detection the emulated device answers polls (it is the only adapter).
	ClearWire();
	ReplaySerialAdapterStatus();
	BOOST_REQUIRE(!Wire().empty());
	BOOST_CHECK(!sa_device->IsEmulationSuppressed());

	// A REAL adapter answers the master at 0x48 (DevStatus reply). The emulated
	// instance observes it and must latch emulation off permanently.
	ReplayDevStatus(static_cast<uint8_t>(SerialAdapter_SystemTemperatureCommands::UNITS), 0x00, 0x00, 0x00);
	BOOST_CHECK(sa_device->IsEmulationSuppressed());

	// From now on, NO further master poll may produce ANY wire output from the
	// emulated instance -- otherwise two transmitters would collide on 0x48.
	ClearWire();
	ReplaySerialAdapterStatus();
	BOOST_CHECK(Wire().empty());

	ClearWire();
	ReplaySerialAdapterStatus();
	BOOST_CHECK(Wire().empty());
}

BOOST_AUTO_TEST_CASE(PresenceGating_SuppressedAdapter_DropsQueuedCommands)
{
	// Detect the real adapter (suppress emulation).
	ReplayDevStatus(static_cast<uint8_t>(SerialAdapter_SystemTemperatureCommands::UNITS), 0x00, 0x00, 0x00);
	BOOST_REQUIRE(sa_device->IsEmulationSuppressed());

	// Any subsequently queued command must be ignored and never reach the wire.
	sa_device->QueueSetpointCommand(SerialAdapter_SystemTemperatureCommands::POOLSP, 82);
	sa_device->QueueAuxToggleWrite(Auxillaries::JandyAuxillaryIds::Aux_1, true);

	ClearWire();
	ReplaySerialAdapterStatus();
	BOOST_CHECK(Wire().empty());
}

BOOST_AUTO_TEST_CASE(PresenceGating_ReadPathStaysLiveAfterSuppression)
{
	// Suppression silences transmission but the device must keep DECODING the real
	// adapter's replies into the DataHub (the whole point of going passive).
	ReplayDevStatus(static_cast<uint8_t>(SerialAdapter_SystemTemperatureCommands::UNITS), 0x00, 0x00, 0x00);
	BOOST_REQUIRE(sa_device->IsEmulationSuppressed());

	ClearWire();
	ReplayDevStatus(static_cast<uint8_t>(SerialAdapter_SystemTemperatureCommands::POOLSP), 0x00, 88, 0x00);

	// No transmission ...
	BOOST_CHECK(Wire().empty());
	// ... but the setpoint was still decoded from the real adapter's reply.
	BOOST_REQUIRE(data_hub->PoolTempSetpoint().has_value());
	BOOST_CHECK_CLOSE(data_hub->PoolTempSetpoint().value().InFahrenheit().value(), 88.0, 0.5);
}

//=============================================================================
// (d) CAPTURE-GATED WRITES round-trip to the exact AqualinkD-derived wire bytes.
//=============================================================================

BOOST_AUTO_TEST_CASE(Write_TwoStepSetpoint_EmitsReadyThenSetOverTwoPolls)
{
	// POOLSP two-step: readySP {ack_type=0x05, data=0x35} then setSP {0x00, 82}.
	sa_device->QueueSetpointWrite_TwoStep(SerialAdapter_SystemTemperatureCommands::POOLSP, 82);

	// Poll 1 -> readySP.
	ClearWire();
	ReplaySerialAdapterStatus();
	BOOST_CHECK(Wire() == ExpectedAckWireBytes(0x05, 0x35));

	// Poll 2 -> setSP (carrying the temperature value 82).
	ClearWire();
	ReplaySerialAdapterStatus();
	BOOST_CHECK(Wire() == ExpectedAckWireBytes(0x00, 82));
}

BOOST_AUTO_TEST_CASE(Write_SpaTwoStepSetpoint_EmitsReadyThenSet)
{
	// SPASP two-step: readySP {0x07, 0x35} then setSP {0x00, 104}.
	sa_device->QueueSetpointWrite_TwoStep(SerialAdapter_SystemTemperatureCommands::SPASP, 104);

	ClearWire();
	ReplaySerialAdapterStatus();
	BOOST_CHECK(Wire() == ExpectedAckWireBytes(0x07, 0x35));

	ClearWire();
	ReplaySerialAdapterStatus();
	BOOST_CHECK(Wire() == ExpectedAckWireBytes(0x00, 104));
}

BOOST_AUTO_TEST_CASE(Write_AuxToggleOn_EmitsSetDevFrame)
{
	// setDev {ack_type=state, data=devID}: Aux_1 ON -> {0x81, 0x15}.
	sa_device->QueueAuxToggleWrite(Auxillaries::JandyAuxillaryIds::Aux_1, true);

	ClearWire();
	ReplaySerialAdapterStatus();
	BOOST_CHECK(Wire() == ExpectedAckWireBytes(0x81, 0x15));
}

BOOST_AUTO_TEST_CASE(Write_AuxToggleOff_EmitsSetDevFrame)
{
	// Aux_2 OFF -> {0x80, 0x16} (0x02 + 0x14 = 0x16).
	sa_device->QueueAuxToggleWrite(Auxillaries::JandyAuxillaryIds::Aux_2, false);

	ClearWire();
	ReplaySerialAdapterStatus();
	BOOST_CHECK(Wire() == ExpectedAckWireBytes(0x80, 0x16));
}

BOOST_AUTO_TEST_SUITE_END()
