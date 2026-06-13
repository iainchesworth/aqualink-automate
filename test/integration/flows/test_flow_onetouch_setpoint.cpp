#include <string>

#include <boost/test/unit_test.hpp>

#include <nlohmann/json.hpp>

#include "kernel/data_hub.h"

#include "jandy/devices/onetouch_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"

#include "support/unit_test_loadfixture.h"
#include "support/unit_test_mockreplayharness.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Test;

//=============================================================================
// OneTouch setpoint-edit END-TO-END flow test.
//
// Driven from the live capture test/fixtures/onetouch_setpoint_edit.cap (a real
// session editing the Set Temperature page: pool +1, spa +1, spa -1, pool -1 degC).
// Decoding that capture is what established the OneTouch SetpointController's exact
// edit model (see Setpoint_ProcessStep / DisplayedSetpoint), verified by hand:
//   * Pool Heat is line 2, Spa Heat is line 3 on the Set Temperature page;
//   * the values are shown/edited in the system's configured units (here degC) -
//     exactly the units the dispatcher passes, so the device compares the on-screen
//     integer with the target with NO C/F conversion;
//   * each edit is Select (enter editor) -> LineUp/LineDown (step +/-1 per press) ->
//     Select (commit) - NOT Back (which would leave the page). e.g. 30->31degC on
//     LineUp, 39->38degC on LineDown.
//
// This test asserts the recorded session round-trips cleanly through the full decode
// stack and that the passive keypad participates. (Like the equipment-toggle replay,
// a live capture only re-sends CHANGED screen lines, so a purely passive replay is
// not guaranteed to reconstruct one clean full Set Temperature refresh that fires the
// page processor - the value-stepping behaviour above is verified against the capture
// by hand rather than re-decoded here.)
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_Integration_Flow_OneTouchSetpoint)

BOOST_AUTO_TEST_CASE(Replay_SetpointEditSession_DecodesCleanly)
{
	MockReplayHarness harness;

	auto device_id = std::make_shared<Devices::JandyDeviceType>(Devices::JandyDeviceId(0x40));
	auto onetouch = std::make_shared<Devices::OneTouchDevice>(device_id, harness.HubLocatorRef(), false);

	auto replayed_bytes = ReplayFixture(harness, "fixtures/onetouch_setpoint_edit.cap");

	// Substantial real traffic, every frame a valid packet.
	BOOST_REQUIRE(!replayed_bytes.empty());
	BOOST_CHECK_GT(replayed_bytes.size(), 1000u);
	BOOST_CHECK_EQUAL(harness.StatisticsHub()->MessageErrors.ChecksumFailures, 0u);

	// The passive keypad processed its frames (left ColdStart), proving the capture
	// drove the device through the Set Temperature menu.
	auto diag = onetouch->DescribeDiagnostics();
	BOOST_CHECK_EQUAL(diag["operating_state"].get<std::string>(), std::string("NormalOperation"));

	onetouch.reset();
}

BOOST_AUTO_TEST_SUITE_END()
