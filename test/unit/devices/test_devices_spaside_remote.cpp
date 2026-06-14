#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/spaside_remote_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_status.h"

#include "support/unit_test_hublocatorinjector.h"
#include "support/unit_test_protocolmessagebuilder.h"
#include "support/unit_test_mockreplayharness.h"
#include "support/unit_test_loadfixture.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Devices;

namespace
{
	// Fire a master->remote poll: a cmd-0x02 frame addressed to `dest`. With a 5-byte payload
	// the framed message is 12 bytes, which is exactly JandyMessage_Status's minimum, so it
	// deserialises as a Status addressed to `dest` (matching the real LED-image poll on the wire).
	void EmitPollTo(uint8_t dest)
	{
		const auto frame = Test::MessageBuilder::CreateValidChecksummedMessage(dest, 0x02, { 0x00, 0x00, 0x00, 0x00, 0x00 });
		Messages::JandyMessage_Status msg;
		const auto span = std::as_bytes(std::span<const uint8_t>(frame.data(), frame.size()));
		BOOST_REQUIRE(msg.Deserialize(span));
		BOOST_REQUIRE_EQUAL(static_cast<unsigned>(msg.Destination().Id()()), static_cast<unsigned>(dest));
		(*Messages::JandyMessage_Status::GetSignal())(msg);
	}

	// Fire a remote->master button report: an Ack (cmd 0x01) addressed to the master (0x00) with
	// ack_type + command (the button index).
	void EmitAck(uint8_t ack_type, uint8_t command)
	{
		const auto frame = Test::MessageBuilder::CreateValidChecksummedMessage(0x00, 0x01, { ack_type, command });
		Messages::JandyMessage_Ack ack;
		const auto span = std::as_bytes(std::span<const uint8_t>(frame.data(), frame.size()));
		BOOST_REQUIRE(ack.Deserialize(span));
		(*Messages::JandyMessage_Ack::GetSignal())(ack);
	}

	struct SpasideFixture : public Test::HubLocatorInjector
	{
		SpasideFixture() : device_type(std::make_shared<JandyDeviceType>(JandyDeviceId(0x10))) {}
		std::shared_ptr<JandyDeviceType> device_type;   // Dual Spa Switch instance 0
	};
}

BOOST_FIXTURE_TEST_SUITE(SpasideRemoteDevice_TestSuite, SpasideFixture)

BOOST_AUTO_TEST_CASE(ButtonPress_AfterPoll_IsDecoded)
{
	SpasideRemoteDevice device(device_type, *this, false);

	EmitPollTo(0x10);     // master polls us -> arms the correlation window
	EmitAck(0x00, 3);     // our reply [0x01][0x00][3] -> button 3

	BOOST_CHECK_EQUAL(static_cast<int>(device.LastButton()), 3);
}

BOOST_AUTO_TEST_CASE(Ack_WithoutPrecedingPoll_IsIgnored)
{
	SpasideRemoteDevice device(device_type, *this, false);

	EmitAck(0x00, 5);     // no poll to us first -> not ours

	BOOST_CHECK_EQUAL(static_cast<int>(device.LastButton()), 0);
}

BOOST_AUTO_TEST_CASE(Ack_WithNonZeroAckType_IsIgnored)
{
	// ack_type 0x80 is a OneTouch-style ACK, not a spa-side button report (which uses 0x00),
	// so even right after our poll it must not be decoded as a button.
	SpasideRemoteDevice device(device_type, *this, false);

	EmitPollTo(0x10);
	EmitAck(0x80, 7);

	BOOST_CHECK_EQUAL(static_cast<int>(device.LastButton()), 0);
}

BOOST_AUTO_TEST_CASE(Poll_ToDifferentAddress_DoesNotArmUs)
{
	// A poll to 0x20 (a different spaside instance) must not arm our 0x10 device, so a following
	// ack is not mis-attributed to us.
	SpasideRemoteDevice device(device_type, *this, false);

	EmitPollTo(0x20);
	EmitAck(0x00, 4);

	BOOST_CHECK_EQUAL(static_cast<int>(device.LastButton()), 0);
}

BOOST_AUTO_TEST_CASE(IdleAck_ButtonZero_RecordsNoPress)
{
	// The steady-state reply when no button is held is [0x01][0x00][0x00]; that must not be
	// recorded as a press.
	SpasideRemoteDevice device(device_type, *this, false);

	EmitPollTo(0x10);
	EmitAck(0x00, 0);

	BOOST_CHECK_EQUAL(static_cast<int>(device.LastButton()), 0);
}

BOOST_AUTO_TEST_CASE(RealCapture_DualSpaSwitch_PolledAndIdle)
{
	// End-to-end against the user's REAL capture: the master heavily polls the Dual Spa Switch
	// at 0x10 (cmd-0x02 LED images), and no spa button was pressed during this recording. Assert
	// the device sees the polls (correlation arms) and decodes no spurious button press.
	Test::MockReplayHarness harness(/*single_read_per_poll=*/true);
	auto id = std::make_shared<JandyDeviceType>(JandyDeviceId(0x10));
	SpasideRemoteDevice device(id, harness.HubLocatorRef(), false);

	Test::ReplayFixture(harness, "fixtures/iaq_aux_setpoint.cap");

	BOOST_CHECK_GT(device.PollCount(), 0u);                       // master polled us on the real bus
	BOOST_CHECK_EQUAL(static_cast<int>(device.LastButton()), 0);  // no press -> no false positive
}

BOOST_AUTO_TEST_SUITE_END()
