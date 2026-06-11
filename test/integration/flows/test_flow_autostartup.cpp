#include <cstdint>
#include <memory>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/startup/jandy_startup_coordinator.h"
#include "jandy/startup/jandy_startup_environment.h"
#include "jandy/startup/jandy_startup_types.h"
#include "kernel/equipment_hub.h"
#include "interfaces/idevice.h"

#include "support/unit_test_mockreplayharness.h"
#include "support/unit_test_protocolmessagebuilder.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Jandy::Startup;
using namespace AqualinkAutomate::Test;

//=============================================================================
// Auto-startup END-TO-END flow test.
//
// Replays a PD-8 Combo cold-boot discovery probe cycle (the address set the real
// master scans, taken from the captured sessions) through the FULL decode stack
// and asserts the auto-startup coordinator makes the same decision it makes live --
// deterministically, with no simulator:
//
//   stand up a SerialAdapter detector -> observe the master's probe set ->
//   classify the controller as PagePush -> stand up an emulated AqualinkTouch.
//
// This closes the loop the live sim couldn't (its polling deadlocked over com0com).
//
// Why a synthesised discovery cycle rather than ReplayFixture(iaq_pages_live.cap):
// that capture was recorded AFTER discovery (the app was already emulating 0x33), so
// its bulk replay routes the 0x30-0x33 frames statelessly-differently and the lone
// post-discovery probe is lost -- it classifies the OneTouch range it does see. A
// clean cold-boot probe cycle is what the live coordinator actually reacts to, and
// it is what is replayed here; the per-frame decode path is identical either way.
//=============================================================================

namespace
{
	// Is a device whose Jandy bus id == `id` present in the harness's EquipmentHub?
	bool HubHasDeviceAt(MockReplayHarness& harness, std::uint8_t id)
	{
		return harness.EquipmentHub()->FindDevice([id](const Interfaces::IDevice& device)
		{
			const auto* jandy_type = dynamic_cast<const Devices::JandyDeviceType*>(&device.DeviceId());
			return (jandy_type != nullptr) && (jandy_type->Id()() == id);
		}) != nullptr;
	}
}

BOOST_AUTO_TEST_SUITE(TestSuite_Integration_Flow_AutoStartup)

BOOST_AUTO_TEST_CASE(Replay_PD8ComboDiscoveryProbes_ClassifiesPagePush_StandsUpAqualinkTouchAt0x33)
{
	MockReplayHarness harness;

	JandyStartupEnvironment environment(harness.HubLocatorRef());
	StartupCoordinator coordinator(environment);

	// Phase 1: stand up the SerialAdapter detector (exactly as the live service does).
	coordinator.Begin();
	BOOST_REQUIRE(coordinator.CurrentPhase() == StartupCoordinator::Phase::Detecting);
	BOOST_REQUIRE(HubHasDeviceAt(harness, 0x48));

	// Replay a faithful PD-8 Combo cold-boot discovery probe cycle (cmd 0x00 to the addresses
	// the real master scans -- the set extracted from the captured sessions, including both the
	// AqualinkTouch 0x30-0x33 and OneTouch 0x40-0x43 ranges) through the FULL decode stack:
	// TestSerialPortImpl -> SerialPort -> ProtocolTask -> Jandy generator -> per-message signal
	// -> the environment's observer. This is the discovery traffic the live coordinator reacts
	// to, replayed deterministically without the simulator.
	const auto cmd_probe = static_cast<std::uint8_t>(Messages::JandyMessageIds::Probe);
	for (std::uint8_t address : { 0x08, 0x18, 0x30, 0x31, 0x32, 0x33, 0x40, 0x41, 0x42, 0x43, 0x48, 0x50, 0x60, 0xA3 })
	{
		harness.Replay(Test::MessageBuilder::CreateValidChecksummedMessage(address, cmd_probe, {}));
	}

	BOOST_CHECK(environment.ObservedProbes().contains(0x33));   // AqualinkTouch range probed

	// Phase 2: classify from the observed bus + stand up the controller emulation.
	auto phase = coordinator.Advance(/*detection_window_elapsed=*/false);

	// AqualinkTouch was probed, so page-push wins over OneTouch spidering, and an emulated
	// AqualinkTouch is stood up at the canonical 0x33; the SerialAdapter detector remains.
	BOOST_CHECK(phase == StartupCoordinator::Phase::Running);
	BOOST_CHECK(coordinator.Plan().method == DataGatheringMethod::PagePush);
	BOOST_CHECK(HubHasDeviceAt(harness, 0x33));
	BOOST_CHECK(HubHasDeviceAt(harness, 0x48));

	// The replayed stream is master-side only (no device->master responses), so nothing was
	// flagged occupied and the controller took the preferred 0x33 slot rather than relocating.
	BOOST_CHECK(environment.OccupiedAddresses().empty());
}

BOOST_AUTO_TEST_SUITE_END()
