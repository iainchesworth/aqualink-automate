#include <cstdint>
#include <memory>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/data_hub.h"

#include "pentair/devices/pentair_chlorinator_device.h"
#include "pentair/devices/pentair_device_id.h"
#include "pentair/messages/chlorinator/pentair_chlorinator_message_setoutput.h"
#include "pentair/messages/chlorinator/pentair_chlorinator_message_status.h"
#include "pentair/messages/pentair_message_ids.h"

#include "support/unit_test_mockreplayharness.h"
#include "support/unit_test_protocolmessagebuilder.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Pentair;
using namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes;

//=============================================================================
// Pentair B3: IntelliChlor (SWG) salt/chlorinator support.
//=============================================================================

namespace
{
	constexpr uint8_t CHLORINATOR = 0x50;
	constexpr uint8_t CONTROLLER = 0x10;
	const uint8_t CMD_STATUS = static_cast<uint8_t>(Messages::PentairMessageIds::Chlorinator_Status);
}

BOOST_AUTO_TEST_SUITE(TestSuite_PentairChlorinator)

//-----------------------------------------------------------------------------
// Decode + surface: a status frame populates salt PPM / output and surfaces a
// Chlorinator auxillary plus the DataHub salt level.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Replay_ChlorinatorStatus_DecodesAndSurfaces)
{
	Test::MockReplayHarness harness;

	auto device_id = std::make_shared<Pentair::Devices::PentairDeviceId>(CHLORINATOR);
	auto& device = harness.AddDevice<Pentair::Devices::PentairChlorinatorDevice>(device_id);

	// salt 64 * 50 = 3200 PPM, output 55%, no fault flags.
	const uint8_t salt_raw = 64;
	const uint8_t output_pct = 55;
	auto frame = Test::PentairMessageBuilder::CreateValidChecksummedFrame(
		CHLORINATOR, CONTROLLER, CMD_STATUS, { salt_raw, output_pct, 0x00 });

	harness.Replay(frame);

	BOOST_TEST(harness.StatisticsHub()->MessageErrors.ChecksumFailures == 0u);

	// Device-level decode.
	BOOST_CHECK_EQUAL(device.ReportedSaltPPM(), 3200u);
	BOOST_CHECK_EQUAL(device.ReportedOutputPercent(), output_pct);

	// Hub-level salt level.
	BOOST_CHECK_EQUAL(harness.DataHub()->SaltLevel().value(), 3200.0);

	// Hub-level chlorinator surface.
	auto chlorinators = harness.DataHub()->Chlorinators();
	BOOST_REQUIRE_EQUAL(chlorinators.size(), 1u);

	auto generating = chlorinators.front()->AuxillaryTraits.TryGet(GeneratingPercentageTrait{});
	BOOST_REQUIRE(generating.has_value());
	BOOST_CHECK_EQUAL(generating.value(), output_pct);

	auto status = chlorinators.front()->AuxillaryTraits.TryGet(ChlorinatorStatusTrait{});
	BOOST_REQUIRE(status.has_value());
	BOOST_CHECK(status.value() == Kernel::ChlorinatorStatuses::On);

	auto health = chlorinators.front()->AuxillaryTraits.TryGet(ChlorinatorHealthTrait{});
	BOOST_REQUIRE(health.has_value());
	BOOST_CHECK(health.value() == Kernel::ChlorinatorHealth::Ok);
}

//-----------------------------------------------------------------------------
// Fault flags map to chlorinator health.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Replay_ChlorinatorLowSaltFlag_MapsToHealth)
{
	Test::MockReplayHarness harness;

	auto device_id = std::make_shared<Pentair::Devices::PentairDeviceId>(CHLORINATOR);
	auto& device = harness.AddDevice<Pentair::Devices::PentairChlorinatorDevice>(device_id);

	// Low-salt flag set (bit0); output 0 -> chlorinator reported Off.
	auto frame = Test::PentairMessageBuilder::CreateValidChecksummedFrame(
		CHLORINATOR, CONTROLLER, CMD_STATUS,
		{ 20, 0, Messages::PentairChlorinatorMessage_Status::FLAG_LOW_SALT });

	harness.Replay(frame);

	auto chlorinators = harness.DataHub()->Chlorinators();
	BOOST_REQUIRE_EQUAL(chlorinators.size(), 1u);

	auto health = chlorinators.front()->AuxillaryTraits.TryGet(ChlorinatorHealthTrait{});
	BOOST_REQUIRE(health.has_value());
	BOOST_CHECK(health.value() == Kernel::ChlorinatorHealth::Warning_LowSalt);

	auto status = chlorinators.front()->AuxillaryTraits.TryGet(ChlorinatorStatusTrait{});
	BOOST_REQUIRE(status.has_value());
	BOOST_CHECK(status.value() == Kernel::ChlorinatorStatuses::Off);
}

//-----------------------------------------------------------------------------
// Command emission: SetOutput emits a set-output command that round-trips
// through the decoder.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(SetOutput_EmitsCommand_RoundTrips)
{
	Test::MockReplayHarness harness;

	auto device_id = std::make_shared<Pentair::Devices::PentairDeviceId>(CHLORINATOR);
	auto& device = harness.AddDevice<Pentair::Devices::PentairChlorinatorDevice>(device_id);

	std::shared_ptr<const Messages::PentairChlorinatorMessage_SetOutput> emitted;
	auto connection = Messages::PentairChlorinatorMessage_SetOutput::GetPublisher()->connect(
		[&emitted](Messages::PentairChlorinatorMessage_SetOutput::PublisherRef ref)
		{
			emitted = std::make_shared<Messages::PentairChlorinatorMessage_SetOutput>(ref.get());
		});

	device.SetOutput(70);
	connection.disconnect();

	BOOST_REQUIRE(emitted != nullptr);
	BOOST_CHECK_EQUAL(emitted->OutputPercent(), 70u);
	BOOST_CHECK_EQUAL(emitted->Destination(), CHLORINATOR);

	// Round-trip the serialised command frame through the decoder.
	std::vector<uint8_t> wire;
	BOOST_REQUIRE(emitted->Serialize(wire));

	std::shared_ptr<const Messages::PentairChlorinatorMessage_SetOutput> decoded;
	auto rx = Messages::PentairChlorinatorMessage_SetOutput::GetSignal()->connect(
		[&decoded](const Messages::PentairChlorinatorMessage_SetOutput& msg)
		{
			decoded = std::make_shared<Messages::PentairChlorinatorMessage_SetOutput>(msg);
		});

	harness.Replay(wire);
	rx.disconnect();

	BOOST_TEST(harness.StatisticsHub()->MessageErrors.ChecksumFailures == 0u);
	BOOST_REQUIRE(decoded != nullptr);
	BOOST_CHECK_EQUAL(decoded->OutputPercent(), 70u);
}

//-----------------------------------------------------------------------------
// SetOutput clamps to 100%.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(SetOutput_ClampsToHundred)
{
	Messages::PentairChlorinatorMessage_SetOutput command(CONTROLLER, CHLORINATOR, 200);
	BOOST_CHECK_EQUAL(command.OutputPercent(), 100u);
}

BOOST_AUTO_TEST_SUITE_END()
