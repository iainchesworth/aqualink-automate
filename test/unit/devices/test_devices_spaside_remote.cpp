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

	// As EmitPollTo, but with a chosen LED-indicator byte as payload[0] (data[1] of the cmd-0x02
	// LED image). The remaining payload bytes are zero. Lets the LED-decode tests drive indicators.
	void EmitLedPollTo(uint8_t dest, uint8_t led_byte)
	{
		const auto frame = Test::MessageBuilder::CreateValidChecksummedMessage(dest, 0x02, { led_byte, 0x00, 0x00, 0x00, 0x00 });
		Messages::JandyMessage_Status msg;
		const auto span = std::as_bytes(std::span<const uint8_t>(frame.data(), frame.size()));
		BOOST_REQUIRE(msg.Deserialize(span));
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

//=============================================================================
// LED-image decode (Phase 3): the master pushes a cmd-0x02 indicator image; the
// first payload byte packs four 2-bit indicators (0=off, 1=on, 2/3=blink).
//=============================================================================

BOOST_AUTO_TEST_CASE(LedImage_AllOff_DecodesAllIndicatorsOff)
{
	SpasideRemoteDevice device(device_type, *this, false);

	BOOST_CHECK(!device.LedImageSeen());   // nothing decoded before the first poll
	EmitLedPollTo(0x10, 0x00);
	BOOST_CHECK(device.LedImageSeen());

	for (std::size_t i = 0; i < 4; ++i)
	{
		BOOST_CHECK(device.Led(i) == SpasideLedState::Off);
	}
}

BOOST_AUTO_TEST_CASE(LedImage_MixedStates_DecodePerIndicator)
{
	SpasideRemoteDevice device(device_type, *this, false);

	// 0b11'10'01'00: LED0=00 off, LED1=01 on, LED2=10 blink, LED3=11 blink.
	EmitLedPollTo(0x10, 0xE4);

	BOOST_CHECK(device.Led(0) == SpasideLedState::Off);
	BOOST_CHECK(device.Led(1) == SpasideLedState::On);
	BOOST_CHECK(device.Led(2) == SpasideLedState::Blink);   // value 2 -> blink
	BOOST_CHECK(device.Led(3) == SpasideLedState::Blink);   // value 3 -> blink
}

BOOST_AUTO_TEST_CASE(LedImage_RetainsRawImageBytes)
{
	SpasideRemoteDevice device(device_type, *this, false);

	EmitLedPollTo(0x10, 0x05);

	// payload[0] is the LED byte; the rest of the 5-byte payload was zero.
	BOOST_REQUIRE(!device.LedImage().empty());
	BOOST_CHECK_EQUAL(static_cast<int>(device.LedImage()[0]), 0x05);
	BOOST_CHECK_EQUAL(device.LedImage().size(), 5u);
}

BOOST_AUTO_TEST_CASE(LedImage_LatestPollWins)
{
	SpasideRemoteDevice device(device_type, *this, false);

	EmitLedPollTo(0x10, 0xFF);   // all blink
	EmitLedPollTo(0x10, 0x00);   // then all off

	for (std::size_t i = 0; i < 4; ++i)
	{
		BOOST_CHECK(device.Led(i) == SpasideLedState::Off);
	}
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

	// The real cmd-0x02 polls carry an LED image -> it was decoded. Validate the decode is
	// self-consistent with the raw byte regardless of which indicators the real system had lit.
	BOOST_REQUIRE(device.LedImageSeen());
	BOOST_REQUIRE(!device.LedImage().empty());

	const uint8_t led_byte = device.LedImage()[0];
	for (std::size_t i = 0; i < 4; ++i)
	{
		const uint8_t bits = static_cast<uint8_t>((led_byte >> (2 * i)) & 0x03);
		const auto expected = (0x00 == bits) ? SpasideLedState::Off
			: (0x01 == bits) ? SpasideLedState::On
			: SpasideLedState::Blink;
		BOOST_CHECK(device.Led(i) == expected);
	}
}

//=============================================================================
// ButtonLayout(): the per-key wire-index -> controller switch:button mapping the
// web layer uses to label and program each key. The 6588 Dual Spa Switch bridges
// two switches (keys 1-4 = Switch 2, keys 5-8 = Switch 3); a Spa Link's mapping
// is undecoded, so its keys are listed but not assignable.
//=============================================================================

BOOST_AUTO_TEST_CASE(ButtonLayout_DualSpaSwitch_MapsKeysToSwitches2And3)
{
	SpasideRemoteDevice device(device_type, *this, true);   // 0x10 Dual Spa Switch (emulated)

	const auto layout = device.ButtonLayout();
	BOOST_REQUIRE_EQUAL(layout.size(), 8u);   // matches ButtonCount()

	for (std::size_t i = 0; i < layout.size(); ++i)
	{
		const auto& b = layout[i];
		BOOST_CHECK_EQUAL(static_cast<int>(b.index), static_cast<int>(i + 1));
		BOOST_CHECK(b.assignable);   // the Dual Spa Switch mapping is decoded
	}

	// Keys 1-4 -> Switch 2 buttons 1-4.
	BOOST_CHECK_EQUAL(static_cast<int>(layout[0].switch_number), 2);
	BOOST_CHECK_EQUAL(static_cast<int>(layout[0].button_number), 1);
	BOOST_CHECK_EQUAL(static_cast<int>(layout[3].switch_number), 2);
	BOOST_CHECK_EQUAL(static_cast<int>(layout[3].button_number), 4);
	// Keys 5-8 -> Switch 3 buttons 1-4.
	BOOST_CHECK_EQUAL(static_cast<int>(layout[4].switch_number), 3);
	BOOST_CHECK_EQUAL(static_cast<int>(layout[4].button_number), 1);
	BOOST_CHECK_EQUAL(static_cast<int>(layout[7].switch_number), 3);
	BOOST_CHECK_EQUAL(static_cast<int>(layout[7].button_number), 4);
}

BOOST_AUTO_TEST_CASE(ButtonLayout_SpaLink_KeysNotAssignable)
{
	// A Spa Link at 0x20: 9 keys, but the key->switch:button mapping is undecoded, so we never
	// fabricate a coordinate -- every key is listed (still pressable) but not assignable.
	auto spalink_id = std::make_shared<JandyDeviceType>(JandyDeviceId(0x20));
	SpasideRemoteDevice device(spalink_id, *this, true);

	const auto layout = device.ButtonLayout();
	BOOST_REQUIRE_EQUAL(layout.size(), 9u);

	for (std::size_t i = 0; i < layout.size(); ++i)
	{
		const auto& b = layout[i];
		BOOST_CHECK_EQUAL(static_cast<int>(b.index), static_cast<int>(i + 1));
		BOOST_CHECK(!b.assignable);
		BOOST_CHECK_EQUAL(static_cast<int>(b.switch_number), 0);
		BOOST_CHECK_EQUAL(static_cast<int>(b.button_number), 0);
	}
}

BOOST_AUTO_TEST_SUITE_END()
