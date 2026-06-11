#include <cstdint>
#include <set>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/startup/jandy_startup_environment.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "interfaces/idevice.h"

#include "support/unit_test_mockreplayharness.h"
#include "support/unit_test_protocolmessagebuilder.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Jandy::Startup;
using DeviceType = AqualinkAutomate::Devices::JandyEmulatedDeviceTypes;

namespace
{
	// Is a device whose Jandy bus id == `id` present in the hub?
	bool HubHasDeviceAt(Test::MockReplayHarness& harness, std::uint8_t id)
	{
		return harness.EquipmentHub()->FindDevice([id](const Interfaces::IDevice& device)
		{
			const auto* jandy_type = dynamic_cast<const Devices::JandyDeviceType*>(&device.DeviceId());
			return (jandy_type != nullptr) && (jandy_type->Id()() == id);
		}) != nullptr;
	}
}

BOOST_AUTO_TEST_SUITE(Jandy_Startup_Environment_TestSuite)

// The production environment, driven through the real protocol stack via the replay harness --
// validating the live wiring deterministically, without the simulator.

BOOST_AUTO_TEST_CASE(ObservedProbes_RecordsTheMastersProbeDestinations)
{
	Test::MockReplayHarness harness;
	JandyStartupEnvironment env(harness.HubLocatorRef());

	const auto cmd_probe = static_cast<std::uint8_t>(Messages::JandyMessageIds::Probe);
	harness.Replay(Test::MessageBuilder::CreateValidChecksummedMessage(0x33, cmd_probe, {}));
	harness.Replay(Test::MessageBuilder::CreateValidChecksummedMessage(0x41, cmd_probe, {}));

	const auto probes = env.ObservedProbes();
	BOOST_CHECK(probes.contains(0x33));   // AqualinkTouch range
	BOOST_CHECK(probes.contains(0x41));   // OneTouch range
}

BOOST_AUTO_TEST_CASE(ObservedProbes_AlsoRecordsActiveControllerPolls)
{
	// A panel that has already discovered its controller addresses it with the controller
	// protocol, not the cold-boot probe: IAQ_Poll (0x30) -> AqualinkTouch, Status (0x02) ->
	// OneTouch. These must count as "addressed as a controller" so classification works on a
	// capture taken after discovery.
	Test::MockReplayHarness harness;
	JandyStartupEnvironment env(harness.HubLocatorRef());

	const auto cmd_iaq_poll = static_cast<std::uint8_t>(Messages::JandyMessageIds::IAQ_Poll);
	harness.Replay(Test::MessageBuilder::CreateValidChecksummedMessage(0x33, cmd_iaq_poll, {}));

	BOOST_CHECK(env.ObservedProbes().contains(0x33));
}

BOOST_AUTO_TEST_CASE(EmulateDevice_StandsTheDeviceUpInTheEquipmentHub)
{
	Test::MockReplayHarness harness;
	JandyStartupEnvironment env(harness.HubLocatorRef());

	BOOST_REQUIRE(!HubHasDeviceAt(harness, 0x33));

	env.EmulateDevice(DeviceType::IAQ, 0x33, "live status via AqualinkTouch page-push");

	BOOST_CHECK(HubHasDeviceAt(harness, 0x33));
}

BOOST_AUTO_TEST_CASE(PanelModelAndRevision_ComeFromEquipmentVersions)
{
	Test::MockReplayHarness harness;
	JandyStartupEnvironment env(harness.HubLocatorRef());

	harness.DataHub()->EquipmentVersions.Set("Model", "PD-8 Combo");
	harness.DataHub()->EquipmentVersions.Set("Revision", "REV T.0.1");

	BOOST_CHECK_EQUAL(env.PanelModel(), "PD-8 Combo");
	BOOST_CHECK_EQUAL(env.PanelRevision(), "REV T.0.1");
}

BOOST_AUTO_TEST_CASE(OccupiedAddresses_EmptyWithNoRealResponders_AndNeverOurOwn)
{
	// With no device->master responses observed, nothing is occupied -- and an address WE
	// emulate must never be reported as occupied even if a response is later attributed to it.
	Test::MockReplayHarness harness;
	JandyStartupEnvironment env(harness.HubLocatorRef());

	env.EmulateDevice(DeviceType::SerialAdapter, 0x48, "detector");

	BOOST_CHECK(env.OccupiedAddresses().empty());
}

BOOST_AUTO_TEST_SUITE_END()
