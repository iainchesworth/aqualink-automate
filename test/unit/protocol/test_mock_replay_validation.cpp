#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/data_hub.h"

#include "jandy/devices/aquarite_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/jandy_message_ids.h"

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

BOOST_AUTO_TEST_SUITE_END()
