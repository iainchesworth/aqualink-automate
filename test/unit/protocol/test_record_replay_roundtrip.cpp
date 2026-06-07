#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/system/error_code.hpp>
#include <boost/test/unit_test.hpp>

#include "developer/recording_serial_port_impl.h"

#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/data_hub.h"

#include "jandy/devices/aquarite_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/jandy_message_ids.h"

#include "mocks/mock_testserialportimpl.h"
#include "support/unit_test_loadfixture.h"
#include "support/unit_test_mockreplayharness.h"
#include "support/unit_test_protocolmessagebuilder.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes;

//=============================================================================
// Record -> replay round-trip + reusable-fixture end-to-end tests.
//
// Proves the recorder and replayer are SYMMETRIC: a session captured by
// RecordingSerialPortImpl can be replayed verbatim (a) byte-for-byte via the
// production MockSerialPortImpl file reader (LoadFixture) and (b) through the
// FULL Jandy decode stack via MockReplayHarness, yielding the same decoded
// DataHub state.  Also exercises the committed sample fixture so downstream
// e2e tests can rely on LoadFixture("fixtures/sample_session.cap").
//=============================================================================

namespace
{
	constexpr uint8_t SWG_DEVICE_ID = 0x50;    // AquaRite salt-water generator.
	constexpr uint8_t MASTER_DEVICE_ID = 0x00; // Aqualink master (PPM destination).

	const uint8_t CMD_AQUARITE_PERCENT = static_cast<uint8_t>(Messages::JandyMessageIds::AQUARITE_Percent);
	const uint8_t CMD_AQUARITE_PPM = static_cast<uint8_t>(Messages::JandyMessageIds::AQUARITE_PPM);
	const uint8_t CMD_AQUARITE_GETID = static_cast<uint8_t>(Messages::JandyMessageIds::AQUARITE_GetId);

	constexpr uint8_t AQUARITE_STATUS_ON = 0x00;

	constexpr uint8_t GENERATING_PERCENT = 40;     // 0x28
	constexpr uint16_t SALT_PPM = 3200;
	constexpr uint8_t SALT_PPM_WIRE = static_cast<uint8_t>(SALT_PPM / 100); // 0x20

	// A unique temp path for the round-trip recording, cleaned up on scope exit.
	struct TempRecordingPath
	{
		std::filesystem::path path;
		TempRecordingPath()
		{
			static int counter = 0;
			path = std::filesystem::temp_directory_path() /
				std::filesystem::path(std::string("aa_roundtrip_") + std::to_string(++counter) + ".cap");
		}
		~TempRecordingPath()
		{
			std::error_code ec;
			std::filesystem::remove(path, ec);
		}
		TempRecordingPath(const TempRecordingPath&) = delete;
		TempRecordingPath& operator=(const TempRecordingPath&) = delete;
	};

	// Build the three AquaRite frames that make up the synthetic session.
	std::vector<uint8_t> GetIdFrame()  { return Test::MessageBuilder::CreateValidChecksummedMessage(SWG_DEVICE_ID, CMD_AQUARITE_GETID, {}); }
	std::vector<uint8_t> PercentFrame(){ return Test::MessageBuilder::CreateValidChecksummedMessage(SWG_DEVICE_ID, CMD_AQUARITE_PERCENT, { GENERATING_PERCENT }); }
	std::vector<uint8_t> PpmFrame()    { return Test::MessageBuilder::CreateValidChecksummedMessage(MASTER_DEVICE_ID, CMD_AQUARITE_PPM, { SALT_PPM_WIRE, AQUARITE_STATUS_ON }); }

	void AssertDecodedAquariteState(Test::MockReplayHarness& harness, AquariteDevice& device)
	{
		BOOST_TEST(harness.StatisticsHub()->MessageErrors.ChecksumFailures == 0u);

		auto chlorinators = harness.DataHub()->Chlorinators();
		BOOST_REQUIRE_EQUAL(chlorinators.size(), 1u);

		auto percentage_trait = chlorinators.front()->AuxillaryTraits.TryGet(GeneratingPercentageTrait{});
		BOOST_REQUIRE(percentage_trait.has_value());
		BOOST_CHECK_EQUAL(percentage_trait.value(), static_cast<uint8_t>(GENERATING_PERCENT));

		BOOST_CHECK_EQUAL(device.ReportedSaltConcentration().value, SALT_PPM);
		BOOST_CHECK_EQUAL(harness.DataHub()->SaltLevel().value(), static_cast<double>(SALT_PPM));
	}
}

BOOST_AUTO_TEST_SUITE(TestSuite_RecordReplayRoundTrip)

//-----------------------------------------------------------------------------
// Capture a synthetic session via RecordingSerialPortImpl, then replay that
// exact recording file (a) byte-for-byte and (b) through the decode stack.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(RecordThenReplay_RoundTrips_BytesAndDecodedState)
{
	const auto getid = GetIdFrame();
	const auto percent = PercentFrame();
	const auto ppm = PpmFrame();

	// Expected replay input = every R-direction byte in capture order.  The W
	// frame written below must NOT appear in the replay stream.
	std::vector<uint8_t> expected_replay;
	expected_replay.insert(expected_replay.end(), getid.begin(), getid.end());
	expected_replay.insert(expected_replay.end(), percent.begin(), percent.end());
	expected_replay.insert(expected_replay.end(), ppm.begin(), ppm.end());

	TempRecordingPath rec;

	// ---- Phase 1: RECORD ---------------------------------------------------
	{
		// A controllable serial impl that returns our synthetic frames as reads.
		auto wrapped = std::make_unique<Test::TestSerialPortImpl>();
		wrapped->EnableTestMode(true);
		boost::system::error_code setup_ec;
		wrapped->set_baud_rate(115200, setup_ec); // fast mock writes
		wrapped->QueueReadData(getid);
		wrapped->QueueReadData(percent);
		wrapped->QueueReadData(ppm);

		Developer::RecordingSerialPortImpl recorder(std::move(wrapped), rec.path.string());

		// Each read pulls one queued frame; the recorder logs it as an R line.
		std::vector<uint8_t> buf(64);
		for (int i = 0; i < 3; ++i)
		{
			boost::system::error_code ec;
			const auto n = recorder.read_some(boost::asio::buffer(buf), ec);
			BOOST_REQUIRE(!ec);
			BOOST_REQUIRE(n > 0);
		}

		// Record a write too (the app polling the SWG) -> logged as a W line that
		// the replayer must ignore.
		boost::system::error_code wec;
		recorder.write_some(boost::asio::buffer(getid), wec);
		BOOST_REQUIRE(!wec);

		// recorder destructor closes + flushes the file here.
	}

	BOOST_REQUIRE(std::filesystem::exists(rec.path));

	// ---- Phase 2a: REPLAY byte-for-byte (LoadFixture) ----------------------
	auto loaded = Test::LoadFixture(rec.path.string());
	BOOST_TEST(loaded == expected_replay, boost::test_tools::per_element());

	// ---- Phase 2b: REPLAY through the full decode stack ---------------------
	Test::MockReplayHarness harness;
	auto device_id = std::make_shared<JandyDeviceType>(JandyDeviceId(SWG_DEVICE_ID));
	auto& device = harness.AddDevice<AquariteDevice>(device_id);

	Test::ReplayFixture(harness, rec.path.string());

	AssertDecodedAquariteState(harness, device);
}

//-----------------------------------------------------------------------------
// The committed sample fixture loads, matches MessageBuilder's framing, and
// decodes into the expected DataHub/EquipmentHub state.  This is the reference
// LoadFixture("fixtures/sample_session.cap") path for downstream e2e tests.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(SampleFixture_LoadsMatchesBuilder_AndDecodes)
{
	const std::string fixture = "fixtures/sample_session.cap";

	// The fixture's R bytes must equal the canonically-built frames; a drift in
	// either the file or the builder fails here loudly.
	std::vector<uint8_t> expected;
	const auto getid = GetIdFrame();
	const auto percent = PercentFrame();
	const auto ppm = PpmFrame();
	expected.insert(expected.end(), getid.begin(), getid.end());
	expected.insert(expected.end(), percent.begin(), percent.end());
	expected.insert(expected.end(), ppm.begin(), ppm.end());

	auto loaded = Test::LoadFixture(fixture);
	BOOST_TEST(loaded == expected, boost::test_tools::per_element());

	// End-to-end: replay the fixture and assert decoded state.
	Test::MockReplayHarness harness;
	auto device_id = std::make_shared<JandyDeviceType>(JandyDeviceId(SWG_DEVICE_ID));
	auto& device = harness.AddDevice<AquariteDevice>(device_id);

	Test::ReplayFixture(harness, fixture);

	AssertDecodedAquariteState(harness, device);
}

//-----------------------------------------------------------------------------
// LoadFixture throws (rather than silently returning empty) for a missing file
// so downstream tests fail fast on a bad path.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(LoadFixture_MissingFile_Throws)
{
	BOOST_CHECK_THROW(Test::LoadFixture("fixtures/does_not_exist_xyz.cap"), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
