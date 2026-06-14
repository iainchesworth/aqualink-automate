#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/iaq_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"

#include "jandy/devices/capabilities/actuation_types.h"

#include "support/unit_test_mockreplayharness.h"
#include "support/unit_test_protocolmessagebuilder.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Devices;

//=============================================================================
// iAQ spa-switch config decode: the controller's "Spa Remotes" page sends the
// button assignments as IAQ_TableMessage (0x26) group-0x00 rows
// "<switch>:<button>\t<function>". The IAQDevice decodes them into the
// controller-agnostic DataHub spa-switch assignment map (read path, Phase 4b).
//=============================================================================

namespace
{
	constexpr uint8_t IAQ_UI_ID = 0x33;   // AqualinkTouch (the iAQ UI half) -- carries pages
	constexpr uint8_t IAQ_TABLE_MESSAGE = 0x26;

	// Build a 0x26 TableMessage payload: [group][row][ "<sw>:<btn>" 0x09 "<function>" 0x00 ].
	std::vector<uint8_t> TableRow(uint8_t group, uint8_t row, const std::string& tag, const std::string& function)
	{
		std::vector<uint8_t> data;
		data.push_back(group);
		data.push_back(row);
		for (char c : tag) { data.push_back(static_cast<uint8_t>(c)); }
		data.push_back(0x09);   // tab separator (sanitised to '?' by the message layer)
		for (char c : function) { data.push_back(static_cast<uint8_t>(c)); }
		data.push_back(0x00);   // NUL terminator
		return data;
	}
}

BOOST_AUTO_TEST_SUITE(IAQSpaSwitchConfig_TestSuite)

BOOST_AUTO_TEST_CASE(TableRows_PopulateAssignmentMap)
{
	Test::MockReplayHarness harness;
	auto id = std::make_shared<JandyDeviceType>(JandyDeviceId(IAQ_UI_ID));
	IAQDevice device(id, harness.HubLocatorRef(), /*is_emulated*/ false);

	// The real "Spa Remotes" page assignment rows (group 0x00).
	harness.Replay({
		Test::MessageBuilder::CreateValidChecksummedMessage(IAQ_UI_ID, IAQ_TABLE_MESSAGE, TableRow(0x00, 1, "1:1", "Spa Jets")),
		Test::MessageBuilder::CreateValidChecksummedMessage(IAQ_UI_ID, IAQ_TABLE_MESSAGE, TableRow(0x00, 2, "1:2", "Pool Light")),
		Test::MessageBuilder::CreateValidChecksummedMessage(IAQ_UI_ID, IAQ_TABLE_MESSAGE, TableRow(0x00, 5, "2:1", "Swim Jet")),
		Test::MessageBuilder::CreateValidChecksummedMessage(IAQ_UI_ID, IAQ_TABLE_MESSAGE, TableRow(0x00, 8, "2:4", "Pool Light")),
	});

	auto hub = harness.DataHub();
	BOOST_REQUIRE(hub->SpaSwitchAssignment(1, 1).has_value());
	BOOST_CHECK_EQUAL(hub->SpaSwitchAssignment(1, 1).value(), "Spa Jets");
	BOOST_CHECK_EQUAL(hub->SpaSwitchAssignment(1, 2).value_or(""), "Pool Light");
	BOOST_CHECK_EQUAL(hub->SpaSwitchAssignment(2, 1).value_or(""), "Swim Jet");
	BOOST_CHECK_EQUAL(hub->SpaSwitchAssignment(2, 4).value_or(""), "Pool Light");
}

BOOST_AUTO_TEST_CASE(FunctionPickerRows_AreIgnored)
{
	// Group-0x01 rows are the assignable-function PICKER, not assignments -- they must not
	// pollute the map. And group-0x00 rows that are not "N:M function" (page headers) are skipped.
	Test::MockReplayHarness harness;
	auto id = std::make_shared<JandyDeviceType>(JandyDeviceId(IAQ_UI_ID));
	IAQDevice device(id, harness.HubLocatorRef(), false);

	harness.Replay({
		Test::MessageBuilder::CreateValidChecksummedMessage(IAQ_UI_ID, IAQ_TABLE_MESSAGE, TableRow(0x01, 1, "Filter Pump", "")),   // picker
		Test::MessageBuilder::CreateValidChecksummedMessage(IAQ_UI_ID, IAQ_TABLE_MESSAGE, TableRow(0x01, 6, "Spa Jets", "")),       // picker
	});

	BOOST_CHECK(harness.DataHub()->SpaSwitchAssignments().empty());
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// iAQ spa-switch WRITE path (SpaSwitchConfigurator). The iAQ "Spa Remotes" page is
// decoded for read + switch-COUNT, but the per-button FUNCTION-reassign command is
// not decoded from any capture (in spaside_setup_nav.cap the maintainer changed only
// the iAQ switch count and did the function edit on the OneTouch). So the iAQ honestly
// reports NotSupported for a function-write, letting the controller fall through to the
// OneTouch. See docs/alwin32/spaside-remotes.md.
//=============================================================================

BOOST_AUTO_TEST_SUITE(IAQSpaSwitchWrite_TestSuite)

BOOST_AUTO_TEST_CASE(SetSpaSwitchAssignment_NotYetDecoded_ReportsNotSupported)
{
	Test::MockReplayHarness harness;
	auto id = std::make_shared<JandyDeviceType>(JandyDeviceId(IAQ_UI_ID));
	IAQDevice device(id, harness.HubLocatorRef(), /*is_emulated*/ true);

	// Even an emulated iAQ cannot program a button function over the bus yet: the wire command
	// is undecoded, so it must report NotSupported rather than fabricate one (or silently no-op).
	BOOST_CHECK(device.SetSpaSwitchAssignment(1, 2, "Pool Light") == Capabilities::ActuationResult::NotSupported);
}

BOOST_AUTO_TEST_CASE(ControllerPriority_IsMedium_DirectChannel)
{
	// The iAQ effects actions via direct page-button/value-submit commands, so it ranks Medium --
	// above the OneTouch (Low). The controller's fall-through ensures that this higher rank does NOT
	// let the (currently NotSupported) iAQ shadow the OneTouch for spa-switch function writes.
	Test::MockReplayHarness harness;
	auto id = std::make_shared<JandyDeviceType>(JandyDeviceId(IAQ_UI_ID));
	IAQDevice device(id, harness.HubLocatorRef(), /*is_emulated*/ true);

	BOOST_CHECK(device.ControllerPriority() == Capabilities::ActuationPriority::Medium);
}

BOOST_AUTO_TEST_SUITE_END()
