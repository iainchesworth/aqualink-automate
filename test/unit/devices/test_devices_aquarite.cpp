#include <chrono>
#include <cstdint>
#include <memory>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/aquarite_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/jandy_message_ids.h"

#include "kernel/auxillary_devices/chlorinator_boost_mode.h"
#include "kernel/auxillary_devices/chlorinator_status.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"

#include "support/unit_test_hublocatorinjector.h"
#include "support/unit_test_mockreplayharness.h"
#include "support/unit_test_ostream_support.h"
#include "support/unit_test_protocolmessagebuilder.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Devices;

namespace
{
	struct AquariteDeviceFixture : public AqualinkAutomate::Test::HubLocatorInjector
	{
		AquariteDeviceFixture()
			: device_type(std::make_shared<JandyDeviceType>(JandyDeviceId(0x50)))
		{
		}

		std::shared_ptr<JandyDeviceType> device_type;
	};
}

BOOST_FIXTURE_TEST_SUITE(AquariteDevice_TestSuite, AquariteDeviceFixture)

// =============================================================================
// Construction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_DefaultValues)
{
	AquariteDevice device(device_type, *this);

	auto requested = device.RequestedGeneratingLevel();
	auto reported = device.ReportedGeneratingLevel();
	auto salt = device.ReportedSaltConcentration();

	BOOST_CHECK_EQUAL(requested.value, static_cast<uint8_t>(0));
	BOOST_CHECK_EQUAL(reported.value, static_cast<uint8_t>(0));
	BOOST_CHECK_EQUAL(salt.value, static_cast<uint16_t>(0));
}

BOOST_AUTO_TEST_CASE(TestConstruction_WithInitialValues)
{
	AquariteDevice device(device_type, *this, 50, 45, 3200);

	auto reported = device.ReportedGeneratingLevel();
	auto salt = device.ReportedSaltConcentration();

	// Note: requested goes through debouncer, so may differ from initial
	BOOST_CHECK_EQUAL(reported.value, static_cast<uint8_t>(45));
	BOOST_CHECK_EQUAL(salt.value, static_cast<uint16_t>(3200));
}

// =============================================================================
// Public API: Setters and Getters
// =============================================================================

BOOST_AUTO_TEST_CASE(TestRequestedGeneratingLevel_SetAndGet_Debounced)
{
	AquariteDevice device(device_type, *this);

	// RequestedGeneratingLevel is debounced (threshold=10), so a single set may not
	// immediately update. Set it repeatedly to exceed the debounce threshold.
	for (uint32_t i = 0; i < 15; ++i)
	{
		device.RequestedGeneratingLevel(75);
	}

	auto result = device.RequestedGeneratingLevel();
	BOOST_CHECK_EQUAL(result.value, static_cast<uint8_t>(75));
}

BOOST_AUTO_TEST_CASE(TestReportedGeneratingLevel_SetAndGet)
{
	AquariteDevice device(device_type, *this);

	device.ReportedGeneratingLevel(60);

	auto result = device.ReportedGeneratingLevel();
	BOOST_CHECK_EQUAL(result.value, static_cast<uint8_t>(60));
}

BOOST_AUTO_TEST_CASE(TestReportedSaltConcentration_SetAndGet)
{
	AquariteDevice device(device_type, *this);

	device.ReportedSaltConcentration(3500);

	auto result = device.ReportedSaltConcentration();
	BOOST_CHECK_EQUAL(result.value, static_cast<uint16_t>(3500));
}

BOOST_AUTO_TEST_CASE(TestReportedGeneratingLevel_TimestampIsRecent)
{
	auto before = std::chrono::system_clock::now();

	AquariteDevice device(device_type, *this);
	device.ReportedGeneratingLevel(50);

	auto after = std::chrono::system_clock::now();

	auto result = device.ReportedGeneratingLevel();
	BOOST_CHECK(result.timestamp >= before);
	BOOST_CHECK(result.timestamp <= after);
}

BOOST_AUTO_TEST_CASE(TestReportedSaltConcentration_TimestampIsRecent)
{
	auto before = std::chrono::system_clock::now();

	AquariteDevice device(device_type, *this);
	device.ReportedSaltConcentration(2800);

	auto after = std::chrono::system_clock::now();

	auto result = device.ReportedSaltConcentration();
	BOOST_CHECK(result.timestamp >= before);
	BOOST_CHECK(result.timestamp <= after);
}

// =============================================================================
// Boundary Values
// =============================================================================

BOOST_AUTO_TEST_CASE(TestReportedGeneratingLevel_ZeroPercent)
{
	AquariteDevice device(device_type, *this);
	device.ReportedGeneratingLevel(0);
	BOOST_CHECK_EQUAL(device.ReportedGeneratingLevel().value, static_cast<uint8_t>(0));
}

BOOST_AUTO_TEST_CASE(TestReportedGeneratingLevel_HundredPercent)
{
	AquariteDevice device(device_type, *this);
	device.ReportedGeneratingLevel(100);
	BOOST_CHECK_EQUAL(device.ReportedGeneratingLevel().value, static_cast<uint8_t>(100));
}

BOOST_AUTO_TEST_CASE(TestReportedSaltConcentration_Zero)
{
	AquariteDevice device(device_type, *this);
	device.ReportedSaltConcentration(0);
	BOOST_CHECK_EQUAL(device.ReportedSaltConcentration().value, static_cast<uint16_t>(0));
}

BOOST_AUTO_TEST_CASE(TestReportedSaltConcentration_MaxPPM)
{
	AquariteDevice device(device_type, *this);
	device.ReportedSaltConcentration(65535);
	BOOST_CHECK_EQUAL(device.ReportedSaltConcentration().value, static_cast<uint16_t>(65535));
}

// =============================================================================
// Multiple Updates
// =============================================================================

BOOST_AUTO_TEST_CASE(TestMultipleUpdates_ReportedGeneratingLevel)
{
	AquariteDevice device(device_type, *this);

	device.ReportedGeneratingLevel(10);
	BOOST_CHECK_EQUAL(device.ReportedGeneratingLevel().value, static_cast<uint8_t>(10));

	device.ReportedGeneratingLevel(50);
	BOOST_CHECK_EQUAL(device.ReportedGeneratingLevel().value, static_cast<uint8_t>(50));

	device.ReportedGeneratingLevel(0);
	BOOST_CHECK_EQUAL(device.ReportedGeneratingLevel().value, static_cast<uint8_t>(0));
}

BOOST_AUTO_TEST_CASE(TestMultipleUpdates_SaltConcentration)
{
	AquariteDevice device(device_type, *this);

	device.ReportedSaltConcentration(2800);
	device.ReportedSaltConcentration(3200);
	device.ReportedSaltConcentration(3500);

	BOOST_CHECK_EQUAL(device.ReportedSaltConcentration().value, static_cast<uint16_t>(3500));
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// DataHub trait clamping regression tests (WU-JANDY-DEVICES).
//
// The AQUARITE_Percent wire byte multiplexes sentinels with the generating
// percentage: 101 == Boost and 255 == Service Mode.  Those sentinels must NOT
// leak into the 0-100 DutyCycle / GeneratingPercentage traits.  These tests
// drive synthetic Percent frames through the full Jandy decode stack via the
// MockReplayHarness and assert the published chlorinator traits stay clamped.
//=============================================================================

namespace
{
	constexpr uint8_t AQUARITE_DEVICE_ID = 0x50;   // SWG / AquaRite address.
	const uint8_t CMD_AQUARITE_PERCENT = static_cast<uint8_t>(AqualinkAutomate::Messages::JandyMessageIds::AQUARITE_Percent);

	using namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes;
}

BOOST_AUTO_TEST_SUITE(AquariteDevice_DataHubClamp_TestSuite)

BOOST_AUTO_TEST_CASE(PercentNormalValue_WritesUnclampedDutyCycle)
{
	Test::MockReplayHarness harness;

	auto device_id = std::make_shared<JandyDeviceType>(JandyDeviceId(AQUARITE_DEVICE_ID));
	harness.AddDevice<AquariteDevice>(device_id);

	// 60% generating level — a normal in-range value.
	auto frame = Test::MessageBuilder::CreateValidChecksummedMessage(AQUARITE_DEVICE_ID, CMD_AQUARITE_PERCENT, { 0x3C });
	harness.Replay(frame);

	auto chlorinators = harness.DataHub()->Chlorinators();
	BOOST_REQUIRE_EQUAL(chlorinators.size(), 1u);

	auto duty = chlorinators.front()->AuxillaryTraits.TryGet(DutyCycleTrait{});
	BOOST_REQUIRE(duty.has_value());
	BOOST_CHECK_EQUAL(duty.value(), static_cast<uint8_t>(60));

	auto generating = chlorinators.front()->AuxillaryTraits.TryGet(GeneratingPercentageTrait{});
	BOOST_REQUIRE(generating.has_value());
	BOOST_CHECK_EQUAL(generating.value(), static_cast<uint8_t>(60));

	auto boost_mode = chlorinators.front()->AuxillaryTraits.TryGet(BoostModeTrait{});
	BOOST_REQUIRE(boost_mode.has_value());
	BOOST_CHECK(boost_mode.value() == Kernel::ChlorinatorBoostModes::Off);
}

BOOST_AUTO_TEST_CASE(PercentBoostSentinel_ClampsDutyCycleAndFlagsBoost)
{
	Test::MockReplayHarness harness;

	auto device_id = std::make_shared<JandyDeviceType>(JandyDeviceId(AQUARITE_DEVICE_ID));
	harness.AddDevice<AquariteDevice>(device_id);

	// 101 (0x65) is the Boost sentinel, NOT a 101% generating level.
	auto frame = Test::MessageBuilder::CreateValidChecksummedMessage(AQUARITE_DEVICE_ID, CMD_AQUARITE_PERCENT, { 0x65 });
	harness.Replay(frame);

	auto chlorinators = harness.DataHub()->Chlorinators();
	BOOST_REQUIRE_EQUAL(chlorinators.size(), 1u);

	// Regression: the raw 101 sentinel must be clamped to 100, never written as-is.
	auto duty = chlorinators.front()->AuxillaryTraits.TryGet(DutyCycleTrait{});
	BOOST_REQUIRE(duty.has_value());
	BOOST_CHECK_EQUAL(duty.value(), static_cast<uint8_t>(100));

	auto generating = chlorinators.front()->AuxillaryTraits.TryGet(GeneratingPercentageTrait{});
	BOOST_REQUIRE(generating.has_value());
	BOOST_CHECK_EQUAL(generating.value(), static_cast<uint8_t>(100));

	// The Boost sentinel must be surfaced through the dedicated BoostMode trait.
	auto boost_mode = chlorinators.front()->AuxillaryTraits.TryGet(BoostModeTrait{});
	BOOST_REQUIRE(boost_mode.has_value());
	BOOST_CHECK(boost_mode.value() == Kernel::ChlorinatorBoostModes::Boost);
}

BOOST_AUTO_TEST_CASE(PercentServiceSentinel_ClampsDutyCycleToZero)
{
	Test::MockReplayHarness harness;

	auto device_id = std::make_shared<JandyDeviceType>(JandyDeviceId(AQUARITE_DEVICE_ID));
	harness.AddDevice<AquariteDevice>(device_id);

	// 255 (0xFF) is the Service Mode sentinel, NOT a 255% generating level.
	auto frame = Test::MessageBuilder::CreateValidChecksummedMessage(AQUARITE_DEVICE_ID, CMD_AQUARITE_PERCENT, { 0xFF });
	harness.Replay(frame);

	auto chlorinators = harness.DataHub()->Chlorinators();
	BOOST_REQUIRE_EQUAL(chlorinators.size(), 1u);

	// Regression: the raw 255 sentinel must NOT be written as a 0-100 percentage.
	auto duty = chlorinators.front()->AuxillaryTraits.TryGet(DutyCycleTrait{});
	BOOST_REQUIRE(duty.has_value());
	BOOST_CHECK_EQUAL(duty.value(), static_cast<uint8_t>(0));

	auto generating = chlorinators.front()->AuxillaryTraits.TryGet(GeneratingPercentageTrait{});
	BOOST_REQUIRE(generating.has_value());
	BOOST_CHECK_EQUAL(generating.value(), static_cast<uint8_t>(0));
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// AQUARITE_PPM output-smoothing regression tests (sticky low-salt).
//
// A borderline AquaRite cell flaps its salt reading AND its low-salt status
// flag around the cell's own internal threshold.  Captured live, the wire
// alternated between "2900 ppm / Warning_LowSalt" (the dominant, true state)
// and short "4000 ppm / On(Ok)" bursts up to ~9 frames long, which the app used
// to surface raw — strobing the salt graph and the chlorinator-health
// indicator.  AquariteDevice now smooths the publish path: a median over the
// last few salt samples, plus an asymmetric dwell on health (raise a warning
// fast, clear it only after it persists).  These tests drive synthetic PPM
// frames matching the capture through the full Jandy decode stack and assert
// the published DataHub state no longer flaps.
//=============================================================================

namespace
{
	constexpr uint8_t AQUARITE_PPM_DEST = 0x00;   // AQUARITE_PPM is sent SWG -> Master (0x00).
	const uint8_t CMD_AQUARITE_PPM = static_cast<uint8_t>(AqualinkAutomate::Messages::JandyMessageIds::AQUARITE_PPM);

	constexpr uint8_t SALT_BYTE_LOW = 0x1d;   // 0x1d * 100 = 2900 ppm (the borderline-low reading).
	constexpr uint8_t SALT_BYTE_OK  = 0x28;   // 0x28 * 100 = 4000 ppm (the transient spike reading).
	constexpr uint8_t STATUS_LOWSALT = 0x02;  // AquariteStatuses::Warning_LowSalt
	constexpr uint8_t STATUS_ON      = 0x00;  // AquariteStatuses::On

	std::vector<uint8_t> MakePpmFrame(uint8_t salt_byte, uint8_t status_byte)
	{
		return Test::MessageBuilder::CreateValidChecksummedMessage(AQUARITE_PPM_DEST, CMD_AQUARITE_PPM, { salt_byte, status_byte });
	}

	void AppendFrames(std::vector<std::vector<uint8_t>>& out, std::size_t count, uint8_t salt_byte, uint8_t status_byte)
	{
		for (std::size_t i = 0; i < count; ++i)
		{
			out.push_back(MakePpmFrame(salt_byte, status_byte));
		}
	}

	Kernel::ChlorinatorHealth HealthOf(const Test::MockReplayHarness& harness)
	{
		auto chlorinators = harness.DataHub()->Chlorinators();
		BOOST_REQUIRE_EQUAL(chlorinators.size(), 1u);
		auto health = chlorinators.front()->AuxillaryTraits.TryGet(AuxillaryTraitsTypes::ChlorinatorHealthTrait{});
		BOOST_REQUIRE(health.has_value());
		return health.value();
	}
}

BOOST_AUTO_TEST_SUITE(AquariteDevice_PpmSmoothing_TestSuite)

BOOST_AUTO_TEST_CASE(IsolatedSaltSpike_IsMedianFiltered_HealthHeldLowSalt)
{
	Test::MockReplayHarness harness;

	auto device_id = std::make_shared<JandyDeviceType>(JandyDeviceId(AQUARITE_DEVICE_ID));
	harness.AddDevice<AquariteDevice>(device_id);

	// 4x low-salt, a single OK spike, then 4x low-salt — exactly the kind of
	// one-frame blip the cell emits on its boundary.
	std::vector<std::vector<uint8_t>> frames;
	AppendFrames(frames, 4, SALT_BYTE_LOW, STATUS_LOWSALT);
	AppendFrames(frames, 1, SALT_BYTE_OK,  STATUS_ON);
	AppendFrames(frames, 4, SALT_BYTE_LOW, STATUS_LOWSALT);
	harness.Replay(frames);

	// The single 4000 spike is outvoted in the median window -> salt reads 2900.
	BOOST_CHECK_CLOSE(harness.DataHub()->SaltLevel().value(), 2900.0, 0.0001);

	// A one-frame OK cannot clear the (sticky) Low-Salt warning.
	BOOST_CHECK_EQUAL(HealthOf(harness), Kernel::ChlorinatorHealth::Warning_LowSalt);
}

BOOST_AUTO_TEST_CASE(ShortOkBurst_DoesNotClearLowSalt)
{
	Test::MockReplayHarness harness;

	auto device_id = std::make_shared<JandyDeviceType>(JandyDeviceId(AQUARITE_DEVICE_ID));
	harness.AddDevice<AquariteDevice>(device_id);

	// A 10-frame OK burst is below the clear threshold (15) -> warning must hold.
	std::vector<std::vector<uint8_t>> frames;
	AppendFrames(frames, 5,  SALT_BYTE_LOW, STATUS_LOWSALT);
	AppendFrames(frames, 10, SALT_BYTE_OK,  STATUS_ON);
	AppendFrames(frames, 5,  SALT_BYTE_LOW, STATUS_LOWSALT);
	harness.Replay(frames);

	BOOST_CHECK_EQUAL(HealthOf(harness), Kernel::ChlorinatorHealth::Warning_LowSalt);
}

BOOST_AUTO_TEST_CASE(SustainedRecovery_ClearsLowSaltAndSaltTracks)
{
	Test::MockReplayHarness harness;

	auto device_id = std::make_shared<JandyDeviceType>(JandyDeviceId(AQUARITE_DEVICE_ID));
	harness.AddDevice<AquariteDevice>(device_id);

	// A genuinely sustained recovery (20 OK frames > 15) DOES clear the warning,
	// and the salt median tracks up to the new sustained value.
	std::vector<std::vector<uint8_t>> frames;
	AppendFrames(frames, 5,  SALT_BYTE_LOW, STATUS_LOWSALT);
	AppendFrames(frames, 20, SALT_BYTE_OK,  STATUS_ON);
	harness.Replay(frames);

	BOOST_CHECK_EQUAL(HealthOf(harness), Kernel::ChlorinatorHealth::Ok);
	BOOST_CHECK_CLOSE(harness.DataHub()->SaltLevel().value(), 4000.0, 0.0001);
}

BOOST_AUTO_TEST_CASE(WarningRaisesQuickly)
{
	Test::MockReplayHarness harness;

	auto device_id = std::make_shared<JandyDeviceType>(JandyDeviceId(AQUARITE_DEVICE_ID));
	harness.AddDevice<AquariteDevice>(device_id);

	// Starting from OK, a warning is surfaced after only a couple of frames.
	std::vector<std::vector<uint8_t>> frames;
	AppendFrames(frames, 3, SALT_BYTE_OK,  STATUS_ON);
	AppendFrames(frames, 2, SALT_BYTE_LOW, STATUS_LOWSALT);
	harness.Replay(frames);

	BOOST_CHECK_EQUAL(HealthOf(harness), Kernel::ChlorinatorHealth::Warning_LowSalt);
}

BOOST_AUTO_TEST_SUITE_END()
