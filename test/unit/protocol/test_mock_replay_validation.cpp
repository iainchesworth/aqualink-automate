#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "kernel/auxillary_devices/chlorinator_status.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/data_hub.h"

#include <stdexcept>

#include <boost/signals2.hpp>

#include "jandy/devices/aquarite_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/aquarite/aquarite_message_percent.h"

#include "support/unit_test_mockreplayharness.h"
#include "support/unit_test_protocolmessagebuilder.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes;

//=============================================================================
// Mock-data replay-and-validate reference test.
//
// This is the canonical demonstration of the MockReplayHarness: feed a known,
// real RS-485 frame sequence (AquaRite / SWG chlorinator traffic) through the
// FULL Jandy decode stack with no hardware, and assert the decoded state both
// on the device object and in the kernel DataHub.
//
// END-TO-END PATH USED: the FULL ProtocolTask.  The harness queues raw bytes
// into Test::TestSerialPortImpl, then drives Protocol::ProtocolTask::Poll(),
// which reads the bytes, runs the registered Jandy message generator (framing +
// checksum validation), and fires the per-message static signal that the
// AquariteDevice's slots are connected to.  No layer is stubbed or bypassed.
//
// Frame references (command ids per JandyMessageIds; checksums computed live by
// MessageBuilder::CreateValidChecksummedMessage):
//   * AQUARITE_Percent (0x11): Master -> SWG (dest 0x50), payload = percentage.
//   * AQUARITE_PPM     (0x16): SWG -> Master (dest 0x00), payload = {PPM/100, status}.
//=============================================================================

namespace
{
	constexpr uint8_t SWG_DEVICE_ID = 0x50;        // AquaRite salt-water generator.
	constexpr uint8_t MASTER_DEVICE_ID = 0x00;     // Aqualink master (PPM destination).

	const uint8_t CMD_AQUARITE_PERCENT = static_cast<uint8_t>(Messages::JandyMessageIds::AQUARITE_Percent);
	const uint8_t CMD_AQUARITE_PPM = static_cast<uint8_t>(Messages::JandyMessageIds::AQUARITE_PPM);

	constexpr uint8_t AQUARITE_STATUS_ON = 0x00;   // Messages::AquariteStatuses::On
}

BOOST_AUTO_TEST_SUITE(TestSuite_MockReplayValidation)

//-----------------------------------------------------------------------------
// Happy path: a real Percent + PPM frame sequence decodes into device + hub
// state.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Replay_AquaritePercentAndPpm_DecodesIntoDeviceAndDataHub)
{
	Test::MockReplayHarness harness;

	// Construct the AquaRite device (id 0x50) inside the harness.  Construction
	// registers its message slots against the static signals, so it will receive
	// the parsed frames we replay below.
	auto device_id = std::make_shared<JandyDeviceType>(JandyDeviceId(SWG_DEVICE_ID));
	auto& device = harness.AddDevice<AquariteDevice>(device_id);

	// ----- Replay a generating-percentage frame (40%) -----------------------
	constexpr uint8_t generating_percent = 40; // 0x28
	auto percent_frame = Test::MessageBuilder::CreateValidChecksummedMessage(
		SWG_DEVICE_ID, CMD_AQUARITE_PERCENT, { generating_percent });

	harness.Replay(percent_frame);

	// The frame must have been a complete, valid packet (no checksum rejection).
	BOOST_TEST(harness.StatisticsHub()->MessageErrors.ChecksumFailures == 0u);

	// Hub-level decoded state: decoding the Percent frame drives the device's
	// Slot_Aquarite_Percent, which creates an AquaPure chlorinator in the DataHub
	// and stamps the generating percentage / duty cycle onto it.  This is the
	// observable end-product of the full decode chain.
	auto chlorinators = harness.DataHub()->Chlorinators();
	BOOST_REQUIRE_EQUAL(chlorinators.size(), 1u);

	auto percentage_trait = chlorinators.front()->AuxillaryTraits.TryGet(GeneratingPercentageTrait{});
	BOOST_REQUIRE(percentage_trait.has_value());
	BOOST_CHECK_EQUAL(percentage_trait.value(), static_cast<uint8_t>(generating_percent));

	auto duty_cycle_trait = chlorinators.front()->AuxillaryTraits.TryGet(DutyCycleTrait{});
	BOOST_REQUIRE(duty_cycle_trait.has_value());
	BOOST_CHECK_EQUAL(duty_cycle_trait.value(), static_cast<uint8_t>(generating_percent));

	// ----- Replay a salt-concentration frame (3200 PPM, status On) ----------
	// On the wire the PPM byte is concentration / 100, so 3200 -> 0x20.
	constexpr uint16_t salt_ppm = 3200;
	constexpr uint8_t salt_ppm_wire = static_cast<uint8_t>(salt_ppm / 100); // 0x20
	auto ppm_frame = Test::MessageBuilder::CreateValidChecksummedMessage(
		MASTER_DEVICE_ID, CMD_AQUARITE_PPM, { salt_ppm_wire, AQUARITE_STATUS_ON });

	harness.Replay(ppm_frame);

	BOOST_TEST(harness.StatisticsHub()->MessageErrors.ChecksumFailures == 0u);

	// Device-level decoded salt concentration.
	BOOST_CHECK_EQUAL(device.ReportedSaltConcentration().value, salt_ppm);

	// Hub-level decoded salt level.
	BOOST_CHECK_EQUAL(harness.DataHub()->SaltLevel().value(), static_cast<double>(salt_ppm));
}

//-----------------------------------------------------------------------------
// Invalid-frame path: a frame with a corrupted checksum is rejected by the
// decode pipeline and must NOT mutate any decoded state.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Replay_CorruptChecksumFrame_IsRejectedAndLeavesStateUntouched)
{
	Test::MockReplayHarness harness;

	auto device_id = std::make_shared<JandyDeviceType>(JandyDeviceId(SWG_DEVICE_ID));
	auto& device = harness.AddDevice<AquariteDevice>(device_id);

	// Build a valid percentage frame, then corrupt the checksum byte so the
	// generator's checksum validation rejects it.  Layout is
	// [DLE STX dest cmd payload checksum DLE ETX] -> checksum is index size-3.
	constexpr uint8_t generating_percent = 75;
	auto bad_frame = Test::MessageBuilder::CreateValidChecksummedMessage(
		SWG_DEVICE_ID, CMD_AQUARITE_PERCENT, { generating_percent });

	const std::size_t checksum_index = bad_frame.size() - 3;
	bad_frame[checksum_index] = static_cast<uint8_t>(bad_frame[checksum_index] ^ 0xFF); // guaranteed wrong

	harness.Replay(bad_frame);

	// The pipeline must have flagged exactly one checksum failure...
	BOOST_TEST(harness.StatisticsHub()->MessageErrors.ChecksumFailures == 1u);

	// ...and no decoded state may have changed: the device stays at its default
	// reported level and no chlorinator was ever created in the DataHub.
	BOOST_CHECK_EQUAL(device.ReportedGeneratingLevel().value, static_cast<uint8_t>(0));
	BOOST_CHECK(harness.DataHub()->Chlorinators().empty());
}

//-----------------------------------------------------------------------------
// Explicit regression lock for the two historical AquaRite wire->DataHub bugs.
//
// Both bugs were fixed in aquarite_device.cpp; this case fails if either is
// re-introduced (verified locally by reverting each fix):
//
//   (1) AquariteMessage_PPM must be registered UNFILTERED.  PPM frames travel
//       SWG->Master with destination 0x00, so a device-id filter on 0x50 would
//       drop them entirely -> SaltLevel would never be set and the chlorinator
//       health/status would stay at their defaults.  (aquarite_device.cpp:68)
//
//   (2) The PPM handler must call ReportedSaltConcentration(), not the
//       generating-level setter.  Calling the wrong setter leaves the reported
//       salt concentration at its default while corrupting the generating
//       level.  (aquarite_device.cpp:157)
//
// It also pins that the chlorinator surfaced by the WIRE path carries the
// "AquaPure" label (the chemistry card / equipment JSON keys off it), which the
// happy-path case above does not assert.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Replay_AquaRite_Regression_UnfilteredPpmAndAquaPureLabel)
{
	Test::MockReplayHarness harness;

	auto device_id = std::make_shared<JandyDeviceType>(JandyDeviceId(SWG_DEVICE_ID));
	auto& device = harness.AddDevice<AquariteDevice>(device_id);

	// Generating percentage (60%) arrives addressed to the SWG (0x50).
	harness.Replay(Test::MessageBuilder::CreateValidChecksummedMessage(
		SWG_DEVICE_ID, CMD_AQUARITE_PERCENT, { static_cast<uint8_t>(60) }));

	// Salt 3000 PPM + status On arrives addressed to the MASTER (0x00).  If the
	// PPM slot were device-id filtered on 0x50 (bug 1) this frame would be
	// dropped and every assertion below on salt / status would fail.
	constexpr uint16_t salt_ppm = 3000;
	harness.Replay(Test::MessageBuilder::CreateValidChecksummedMessage(
		MASTER_DEVICE_ID, CMD_AQUARITE_PPM,
		{ static_cast<uint8_t>(salt_ppm / 100), AQUARITE_STATUS_ON }));

	BOOST_TEST(harness.StatisticsHub()->MessageErrors.ChecksumFailures == 0u);

	auto chlorinators = harness.DataHub()->Chlorinators();
	BOOST_REQUIRE_EQUAL(chlorinators.size(), 1u);
	auto& chlorinator = chlorinators.front();

	// The wire path must stamp the "AquaPure" label.
	auto label = chlorinator->AuxillaryTraits.TryGet(LabelTrait{});
	BOOST_REQUIRE(label.has_value());
	BOOST_CHECK_EQUAL(label.value(), std::string{ "AquaPure" });

	// Bug 2 lock: reported salt concentration lands via ReportedSaltConcentration.
	BOOST_CHECK_EQUAL(device.ReportedSaltConcentration().value, salt_ppm);

	// Bug 1 lock: the unfiltered PPM frame reached the DataHub salt level...
	BOOST_CHECK_EQUAL(harness.DataHub()->SaltLevel().value(), static_cast<double>(salt_ppm));

	// ...and drove the chlorinator operating status On (status byte On).
	auto status = chlorinator->AuxillaryTraits.TryGet(ChlorinatorStatusTrait{});
	BOOST_REQUIRE(status.has_value());
	// ChlorinatorStatuses has no ostream operator<<, so compare with == rather
	// than BOOST_CHECK_EQUAL (which streams both operands on failure).
	BOOST_CHECK(status.value() == Kernel::ChlorinatorStatuses::On);
}

//-----------------------------------------------------------------------------
// Exception barrier: a throwing message-handler slot must NOT escape the serial
// poll loop and terminate the daemon.
//
// Regression for the remote-DoS gap where ProcessMessages() fanned a decoded
// frame out to its signal slots with no try/catch: any slot that threw
// (out_of_range, bad_variant_access, bad_alloc, …) propagated out of the poll
// loop and killed the whole process — reachable from the RS-485 / remote-serial
// transport.  The barrier in ProtocolTask::ProcessMessages must now catch the
// exception, count it (StatisticsHub.MessageErrors.HandlerExceptions), and keep
// polling.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Replay_ThrowingHandlerSlot_IsContainedAndCounted)
{
	Test::MockReplayHarness harness;

	// Connect a slot that throws when the decoded Percent frame is dispatched.
	// scoped_connection auto-disconnects at end of scope so the throwing slot does
	// not leak into other cases that share this process-global static signal.
	boost::signals2::scoped_connection throwing_slot =
		Messages::AquariteMessage_Percent::GetSignal()->connect(
			[](const Messages::AquariteMessage_Percent&)
			{
				throw std::runtime_error("simulated handler fault");
			});

	auto percent_frame = Test::MessageBuilder::CreateValidChecksummedMessage(
		SWG_DEVICE_ID, CMD_AQUARITE_PERCENT, { static_cast<uint8_t>(40) });

	// The replay drives ProtocolTask::Poll(); the throw from the slot must be
	// contained — Replay (and thus the daemon's poll loop) must not propagate it.
	BOOST_CHECK_NO_THROW(harness.Replay(percent_frame));

	// The frame itself was valid (no checksum rejection) and the handler fault was
	// caught and tallied exactly once.
	BOOST_TEST(harness.StatisticsHub()->MessageErrors.ChecksumFailures == 0u);
	BOOST_TEST(harness.StatisticsHub()->MessageErrors.HandlerExceptions == 1u);
}

BOOST_AUTO_TEST_SUITE_END()
