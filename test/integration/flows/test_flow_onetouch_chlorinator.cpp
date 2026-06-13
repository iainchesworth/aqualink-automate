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
// OneTouch chlorinator END-TO-END flow test.
//
// Driven from the live capture test/fixtures/onetouch_chlorinator.cap (Set AquaPure
// pool/spa % edited in 5% steps, then a Boost Pool start/stop). Decoding that capture
// established the OneTouch ChlorinatorController behaviour (see ValueEdit_ProcessStep
// / Boost_ProcessStep), verified by hand:
//   * Set AquaPure: Pool % = line 3, Spa % = line 4; same in-place value editor as the
//     setpoint (Select-enter -> arrow -> Select-commit) but in 5% steps (40->45 on
//     LineUp, 45->40 on LineDown). The single-% interface drives the POOL row.
//   * Boost Pool: START = Select on the "Operate the chlorinator at 100%" page; STOP =
//     navigate to the running-page "Stop" submenu item + Select (user-confirmed the
//     pump stopped).
//
// This test asserts the recorded session round-trips cleanly through the full decode
// stack and that the passive keypad participates. (As with the setpoint/toggle captures,
// a passive replay only re-sends changed screen lines, so the value-editor behaviour
// above is verified against the capture by hand rather than re-decoded here.)
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_Integration_Flow_OneTouchChlorinator)

BOOST_AUTO_TEST_CASE(Replay_ChlorinatorEditSession_DecodesCleanly)
{
	MockReplayHarness harness;

	auto device_id = std::make_shared<Devices::JandyDeviceType>(Devices::JandyDeviceId(0x40));
	auto onetouch = std::make_shared<Devices::OneTouchDevice>(device_id, harness.HubLocatorRef(), false);

	auto replayed_bytes = ReplayFixture(harness, "fixtures/onetouch_chlorinator.cap");

	BOOST_REQUIRE(!replayed_bytes.empty());
	BOOST_CHECK_GT(replayed_bytes.size(), 1000u);
	BOOST_CHECK_EQUAL(harness.StatisticsHub()->MessageErrors.ChecksumFailures, 0u);

	// The passive keypad processed its frames (left ColdStart), proving the capture drove
	// the device through the Set AquaPure + Boost Pool menus.
	auto diag = onetouch->DescribeDiagnostics();
	BOOST_CHECK_EQUAL(diag["operating_state"].get<std::string>(), std::string("NormalOperation"));

	onetouch.reset();
}

BOOST_AUTO_TEST_SUITE_END()
