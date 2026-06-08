#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/data_hub.h"

#include "jandy/devices/aquarite_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"

#include "support/unit_test_loadfixture.h"
#include "support/unit_test_mockreplayharness.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes;
using namespace AqualinkAutomate::Test;

//=============================================================================
// Fixture-replay end-to-end flow tests.
//
// These tests take a RECORDED RS-485 capture file (the developer recorder's
// on-disk format under test/fixtures/) and replay it through the FULL Jandy
// decode stack with no hardware, then assert the resulting decoded kernel
// state.  This is the highest-fidelity replay we can do: LoadFixture parses the
// capture with the SAME production reader used by `--dev-mode --replay-filename`
// (so the bytes are byte-for-byte what a live replay sees), and MockReplayHarness
// drives the real Protocol::ProtocolTask (read -> circular buffer -> Jandy
// message generator -> per-message static signal -> device slot -> kernel hubs).
//
// Distinction from test/unit/protocol/test_mock_replay_validation.cpp: that test
// builds frames in-memory via MessageBuilder.  Here the frames come from an
// on-disk capture, so we additionally exercise the capture-file parse path and
// prove a recorded session round-trips into the expected DataHub/EquipmentHub
// state — the e2e "replay a real session" requirement.
//=============================================================================

namespace
{
	constexpr uint8_t SWG_DEVICE_ID = 0x50;	// AquaRite salt-water generator.
}

BOOST_AUTO_TEST_SUITE(TestSuite_Integration_Flow_FixtureReplay)

//-----------------------------------------------------------------------------
// The reference fixture (sample_session.cap) decodes a single 40% / 3200 PPM
// AquaRite snapshot into a chlorinator in the DataHub.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Replay_SampleSession_PopulatesChlorinatorInDataHub)
{
	MockReplayHarness harness;

	// Construct the AquaRite device (id 0x50) inside the harness.  Construction
	// registers its message slots, so it will receive the replayed frames.
	auto device_id = std::make_shared<JandyDeviceType>(JandyDeviceId(SWG_DEVICE_ID));
	auto& device = harness.AddDevice<AquariteDevice>(device_id);

	// Replay the capture file through the full stack.
	auto replayed_bytes = ReplayFixture(harness, "fixtures/sample_session.cap");

	// Sanity: the fixture contained replayable (R-direction) bytes.
	BOOST_REQUIRE(!replayed_bytes.empty());

	// Every frame in the fixture was a complete, valid packet.
	BOOST_TEST(harness.StatisticsHub()->MessageErrors.ChecksumFailures == 0u);

	// ----- DataHub: a chlorinator was created from the decoded frames --------
	auto chlorinators = harness.DataHub()->Chlorinators();
	BOOST_REQUIRE_EQUAL(chlorinators.size(), 1u);

	// GeneratingPercentage == 40 (from the AQUARITE_Percent frame).
	auto percentage_trait = chlorinators.front()->AuxillaryTraits.TryGet(GeneratingPercentageTrait{});
	BOOST_REQUIRE(percentage_trait.has_value());
	BOOST_CHECK_EQUAL(percentage_trait.value(), static_cast<uint8_t>(40));

	// SaltLevel == 3200 PPM (from the AQUARITE_PPM frame).
	BOOST_CHECK_EQUAL(harness.DataHub()->SaltLevel().value(), 3200.0);

	// Device-level salt concentration matches the hub-level value.
	BOOST_CHECK_EQUAL(device.ReportedSaltConcentration().value, static_cast<uint16_t>(3200));
}

//-----------------------------------------------------------------------------
// The richer fixture (chlorinator_transitions.cap) changes the generating
// level and salt twice; the decoded state must reflect the LAST frame of each
// kind (40% -> 60%, 3200 -> 3000 PPM), proving every frame is applied in order.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Replay_TransitionsSession_DecodesFinalStateAfterMultipleUpdates)
{
	MockReplayHarness harness;

	auto device_id = std::make_shared<JandyDeviceType>(JandyDeviceId(SWG_DEVICE_ID));
	auto& device = harness.AddDevice<AquariteDevice>(device_id);

	auto replayed_bytes = ReplayFixture(harness, "fixtures/chlorinator_transitions.cap");
	BOOST_REQUIRE(!replayed_bytes.empty());

	BOOST_TEST(harness.StatisticsHub()->MessageErrors.ChecksumFailures == 0u);

	auto chlorinators = harness.DataHub()->Chlorinators();
	BOOST_REQUIRE_EQUAL(chlorinators.size(), 1u);

	// Final generating level is the SECOND Percent frame's value (60%).
	auto percentage_trait = chlorinators.front()->AuxillaryTraits.TryGet(GeneratingPercentageTrait{});
	BOOST_REQUIRE(percentage_trait.has_value());
	BOOST_CHECK_EQUAL(percentage_trait.value(), static_cast<uint8_t>(60));

	// Final salt level is the SECOND PPM frame's value (3000 PPM).
	BOOST_CHECK_EQUAL(harness.DataHub()->SaltLevel().value(), 3000.0);

	BOOST_CHECK_EQUAL(device.ReportedSaltConcentration().value, static_cast<uint16_t>(3000));
}

//-----------------------------------------------------------------------------
// LoadFixture must skip W-direction (application-output) lines: the fixtures
// contain a captured W frame on purpose.  The replayable byte count must equal
// only the R-direction frames, so a future regression that starts feeding W
// bytes back into the decoder would change this count and fail here.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Replay_LoadFixture_SkipsWriteDirectionLines)
{
	// sample_session.cap R-lines:
	//   GetId   : 7 bytes
	//   Percent : 8 bytes
	//   PPM     : 9 bytes
	// = 24 replayable bytes; the single W line (7 bytes) must be excluded.
	auto bytes = LoadFixture("fixtures/sample_session.cap");
	BOOST_CHECK_EQUAL(bytes.size(), 24u);

	// The first replayable byte is the DLE that starts the first R frame, not
	// anything from the W line.
	BOOST_REQUIRE(!bytes.empty());
	BOOST_CHECK_EQUAL(bytes.front(), static_cast<uint8_t>(0x10));
}

BOOST_AUTO_TEST_SUITE_END()
