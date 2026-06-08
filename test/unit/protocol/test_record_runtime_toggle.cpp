#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>
#include <boost/test/unit_test.hpp>

#include "developer/recording_serial_port_impl.h"
#include "interfaces/irecordingcontroller.h"

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
// Runtime start/stop toggle for RecordingSerialPortImpl.
//
// Proves the production-safe runtime recording behaviour:
//   (a) Constructed via the pass-through ctor the decorator records NOTHING and
//       writes no file until StartRecording() is called.
//   (b) Once started, traffic is captured in the recorder format, and the
//       resulting file replays byte-for-byte through the production
//       MockSerialPortImpl reader and decodes through the full Jandy stack.
//   (c) StopRecording() halts capture; bytes read afterwards are not recorded.
//   (d) The IRecordingController status (recording flag / file / bytes) tracks
//       the toggle.
//=============================================================================

namespace
{
	constexpr uint8_t SWG_DEVICE_ID = 0x50;
	constexpr uint8_t MASTER_DEVICE_ID = 0x00;

	const uint8_t CMD_AQUARITE_PERCENT = static_cast<uint8_t>(Messages::JandyMessageIds::AQUARITE_Percent);
	const uint8_t CMD_AQUARITE_PPM = static_cast<uint8_t>(Messages::JandyMessageIds::AQUARITE_PPM);
	const uint8_t CMD_AQUARITE_GETID = static_cast<uint8_t>(Messages::JandyMessageIds::AQUARITE_GetId);

	constexpr uint8_t AQUARITE_STATUS_ON = 0x00;
	constexpr uint8_t GENERATING_PERCENT = 40;
	constexpr uint16_t SALT_PPM = 3200;
	constexpr uint8_t SALT_PPM_WIRE = static_cast<uint8_t>(SALT_PPM / 100);

	struct TempRecordingPath
	{
		std::filesystem::path path;
		TempRecordingPath()
		{
			static int counter = 0;
			path = std::filesystem::temp_directory_path() /
				std::filesystem::path(std::string("aa_runtime_rec_") + std::to_string(++counter) + ".cap");
		}
		~TempRecordingPath()
		{
			std::error_code ec;
			std::filesystem::remove(path, ec);
		}
		TempRecordingPath(const TempRecordingPath&) = delete;
		TempRecordingPath& operator=(const TempRecordingPath&) = delete;
	};

	std::vector<uint8_t> GetIdFrame()  { return Test::MessageBuilder::CreateValidChecksummedMessage(SWG_DEVICE_ID, CMD_AQUARITE_GETID, {}); }
	std::vector<uint8_t> PercentFrame(){ return Test::MessageBuilder::CreateValidChecksummedMessage(SWG_DEVICE_ID, CMD_AQUARITE_PERCENT, { GENERATING_PERCENT }); }
	std::vector<uint8_t> PpmFrame()    { return Test::MessageBuilder::CreateValidChecksummedMessage(MASTER_DEVICE_ID, CMD_AQUARITE_PPM, { SALT_PPM_WIRE, AQUARITE_STATUS_ON }); }

	std::size_t ReadOneFrame(Interfaces::ISerialPortImpl& port)
	{
		std::vector<uint8_t> buf(64);
		boost::system::error_code ec;
		const auto n = port.read_some(boost::asio::buffer(buf), ec);
		BOOST_REQUIRE(!ec);
		return n;
	}
}

BOOST_AUTO_TEST_SUITE(TestSuite_RecordRuntimeToggle)

//-----------------------------------------------------------------------------
// Pass-through ctor: nothing recorded, no file, until StartRecording().
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(PassThrough_RecordsNothing_UntilStarted)
{
	TempRecordingPath rec;

	auto wrapped = std::make_unique<Test::TestSerialPortImpl>();
	wrapped->EnableTestMode(true);
	boost::system::error_code setup_ec;
	wrapped->set_baud_rate(115200, setup_ec);
	wrapped->QueueReadData(GetIdFrame());   // consumed while NOT recording
	wrapped->QueueReadData(PercentFrame()); // consumed while recording
	wrapped->QueueReadData(PpmFrame());     // consumed while recording

	Developer::RecordingSerialPortImpl recorder(std::move(wrapped));

	// Default state: pass-through, no file on disk, status reports idle.
	BOOST_CHECK(!recorder.IsRecording());
	auto idle_status = recorder.RecordingStatus();
	BOOST_CHECK(!idle_status.recording);
	BOOST_CHECK(idle_status.file.empty());
	BOOST_CHECK_EQUAL(idle_status.bytes_written, 0u);

	// Read a frame while NOT recording -> must not create a file.
	BOOST_REQUIRE(ReadOneFrame(recorder) > 0);
	BOOST_CHECK(!std::filesystem::exists(rec.path));

	// Start recording, capture the remaining two frames.
	BOOST_REQUIRE(recorder.StartRecording(rec.path.string()));
	BOOST_CHECK(recorder.IsRecording());

	const auto percent = PercentFrame();
	const auto ppm = PpmFrame();
	BOOST_REQUIRE(ReadOneFrame(recorder) > 0);
	BOOST_REQUIRE(ReadOneFrame(recorder) > 0);

	auto active_status = recorder.RecordingStatus();
	BOOST_CHECK(active_status.recording);
	BOOST_CHECK_EQUAL(active_status.file, rec.path.string());
	BOOST_CHECK_EQUAL(active_status.bytes_written, percent.size() + ppm.size());

	BOOST_REQUIRE(recorder.StopRecording());
	BOOST_CHECK(!recorder.IsRecording());

	BOOST_REQUIRE(std::filesystem::exists(rec.path));

	// Only the frames captured AFTER StartRecording are in the file; the first
	// (pre-start) frame must be absent.
	std::vector<uint8_t> expected;
	expected.insert(expected.end(), percent.begin(), percent.end());
	expected.insert(expected.end(), ppm.begin(), ppm.end());

	auto loaded = Test::LoadFixture(rec.path.string());
	BOOST_TEST(loaded == expected, boost::test_tools::per_element());
}

//-----------------------------------------------------------------------------
// A runtime-captured file replays through the full Jandy decode stack.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(RuntimeCapture_ReplaysThroughDecodeStack)
{
	TempRecordingPath rec;

	auto wrapped = std::make_unique<Test::TestSerialPortImpl>();
	wrapped->EnableTestMode(true);
	boost::system::error_code setup_ec;
	wrapped->set_baud_rate(115200, setup_ec);
	wrapped->QueueReadData(GetIdFrame());
	wrapped->QueueReadData(PercentFrame());
	wrapped->QueueReadData(PpmFrame());

	Developer::RecordingSerialPortImpl recorder(std::move(wrapped));
	BOOST_REQUIRE(recorder.StartRecording(rec.path.string()));

	for (int i = 0; i < 3; ++i)
	{
		BOOST_REQUIRE(ReadOneFrame(recorder) > 0);
	}

	// A write while recording is captured as a W line that replay must ignore.
	boost::system::error_code wec;
	const auto getid = GetIdFrame();
	recorder.write_some(boost::asio::buffer(getid), wec);
	BOOST_REQUIRE(!wec);

	BOOST_REQUIRE(recorder.StopRecording());

	Test::MockReplayHarness harness;
	auto device_id = std::make_shared<JandyDeviceType>(JandyDeviceId(SWG_DEVICE_ID));
	auto& device = harness.AddDevice<AquariteDevice>(device_id);

	Test::ReplayFixture(harness, rec.path.string());

	BOOST_TEST(harness.StatisticsHub()->MessageErrors.ChecksumFailures == 0u);

	auto chlorinators = harness.DataHub()->Chlorinators();
	BOOST_REQUIRE_EQUAL(chlorinators.size(), 1u);

	auto percentage_trait = chlorinators.front()->AuxillaryTraits.TryGet(GeneratingPercentageTrait{});
	BOOST_REQUIRE(percentage_trait.has_value());
	BOOST_CHECK_EQUAL(percentage_trait.value(), static_cast<uint8_t>(GENERATING_PERCENT));

	BOOST_CHECK_EQUAL(device.ReportedSaltConcentration().value, SALT_PPM);
	BOOST_CHECK_EQUAL(harness.DataHub()->SaltLevel().value(), static_cast<double>(SALT_PPM));
}

//-----------------------------------------------------------------------------
// Stop is idempotent-ish: a second stop returns false; start after stop returns
// true and truncates a fresh capture.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(StartStop_StateMachine)
{
	TempRecordingPath rec1;
	TempRecordingPath rec2;

	auto wrapped = std::make_unique<Test::TestSerialPortImpl>();
	wrapped->EnableTestMode(true);
	boost::system::error_code setup_ec;
	wrapped->set_baud_rate(115200, setup_ec);
	wrapped->QueueReadData(PercentFrame());
	wrapped->QueueReadData(PpmFrame());

	Developer::RecordingSerialPortImpl recorder(std::move(wrapped));

	// Stop with nothing recording -> false.
	BOOST_CHECK(!recorder.StopRecording());

	// First session.
	BOOST_REQUIRE(recorder.StartRecording(rec1.path.string()));
	// Double-start is rejected.
	BOOST_CHECK(!recorder.StartRecording(rec2.path.string()));
	BOOST_REQUIRE(ReadOneFrame(recorder) > 0);
	BOOST_REQUIRE(recorder.StopRecording());
	// Double-stop -> false.
	BOOST_CHECK(!recorder.StopRecording());

	BOOST_REQUIRE(std::filesystem::exists(rec1.path));

	// Second session writes to a different file and starts cleanly.
	BOOST_REQUIRE(recorder.StartRecording(rec2.path.string()));
	BOOST_CHECK(recorder.IsRecording());
	BOOST_REQUIRE(ReadOneFrame(recorder) > 0);
	auto status = recorder.RecordingStatus();
	BOOST_CHECK_EQUAL(status.file, rec2.path.string());
	BOOST_REQUIRE(recorder.StopRecording());
}

BOOST_AUTO_TEST_SUITE_END()
