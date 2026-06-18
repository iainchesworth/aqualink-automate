#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <boost/signals2.hpp>
#include <boost/test/unit_test.hpp>

#include "jandy/devices/iaq_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/jandy_message_ack.h"

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

	constexpr uint8_t IAQ_PAGE_START = 0x23;
	constexpr uint8_t IAQ_PAGE_END = 0x28;
	constexpr uint8_t IAQ_POLL = 0x30;
	constexpr uint8_t IAQ_PAGE_SPA_SWITCH_DETAIL = 0x3b;

	// A group-0x01 PICKER row: [0x01][slot][ "<function>" 0x00 ]. Slot 0 is the "Devices /" header
	// that resets the picker (so a scrolled page replaces, not accumulates).
	std::vector<uint8_t> PickerRow(uint8_t slot, const std::string& function)
	{
		std::vector<uint8_t> data{ 0x01, slot };
		for (char c : function) { data.push_back(static_cast<uint8_t>(c)); }
		data.push_back(0x00);
		return data;
	}

	using Frame = std::vector<uint8_t>;
	Frame PageStart(uint8_t page_id)
	{
		return Test::MessageBuilder::CreateValidChecksummedMessage(0x33, IAQ_PAGE_START, { page_id });
	}
	Frame PageEnd()
	{
		return Test::MessageBuilder::CreateValidChecksummedMessage(0x33, IAQ_PAGE_END, { 0x06, 0x0e });
	}
	Frame Poll()
	{
		return Test::MessageBuilder::CreateValidChecksummedMessage(0x33, IAQ_POLL, { 0x00 });
	}
	Frame Table(const std::vector<uint8_t>& payload)
	{
		return Test::MessageBuilder::CreateValidChecksummedMessage(0x33, IAQ_TABLE_MESSAGE, payload);
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
// iAQ spa-switch WRITE path (SpaSwitchConfigurator). The iAQ programs a button's
// function by driving the AqualinkTouch "4 Function Spa Switch" detail page: select
// the S:B row (page-button ordinal+4), scroll the device PICKER (0x15) until the
// target function is visible, then commit at its slot (page-button slot+11). RE'd +
// cross-validated from captures/iaq_spaswitch_edit{,2}.cap; see docs/alwin32/spaside-remotes.md.
//=============================================================================

namespace
{
	// Replay the 4-Function detail page: PageStart(0x3b) + group-0 assignments (1:2 carries
	// `fn12`) + the group-1 device PICKER `picker` (slot -> function) + PageEnd.
	void ReplayDetail(Test::MockReplayHarness& harness, const std::string& fn12, const std::vector<std::pair<uint8_t, std::string>>& picker)
	{
		std::vector<Frame> frames;
		frames.push_back(PageStart(IAQ_PAGE_SPA_SWITCH_DETAIL));
		frames.push_back(Table(TableRow(0x00, 1, "1:1", "Spa Jets")));
		frames.push_back(Table(TableRow(0x00, 2, "1:2", fn12)));
		frames.push_back(Table(TableRow(0x00, 5, "2:1", "Swim Jet")));
		frames.push_back(Table(TableRow(0x00, 6, "2:2", "Swim Jet")));
		frames.push_back(Table(PickerRow(0, "Devices /")));   // header -> resets the picker map
		for (const auto& [slot, fn] : picker) { frames.push_back(Table(PickerRow(slot, fn))); }
		frames.push_back(PageEnd());
		harness.Replay(frames);
	}

	// Replay a fresh picker batch in place (e.g. after a scroll): header (reset) + the new rows.
	void ReplayPicker(Test::MockReplayHarness& harness, const std::vector<std::pair<uint8_t, std::string>>& picker)
	{
		std::vector<Frame> frames;
		frames.push_back(Table(PickerRow(0, "Devices /")));
		for (const auto& [slot, fn] : picker) { frames.push_back(Table(PickerRow(slot, fn))); }
		harness.Replay(frames);
	}
}

BOOST_AUTO_TEST_SUITE(IAQSpaSwitchWrite_TestSuite)

BOOST_AUTO_TEST_CASE(SetSpaSwitchAssignment_Emulated_ValidRequest_IsAccepted)
{
	Test::MockReplayHarness harness;
	auto id = std::make_shared<JandyDeviceType>(JandyDeviceId(IAQ_UI_ID));
	IAQDevice device(id, harness.HubLocatorRef(), /*is_emulated*/ true);

	BOOST_CHECK(device.SetSpaSwitchAssignment(1, 2, "Pool Heat") == Capabilities::ActuationResult::Accepted);
}

BOOST_AUTO_TEST_CASE(SetSpaSwitchAssignment_NotEmulated_IsNotSupported)
{
	// A passive iAQ never transmits, so it cannot program -- the controller falls through elsewhere.
	Test::MockReplayHarness harness;
	auto id = std::make_shared<JandyDeviceType>(JandyDeviceId(IAQ_UI_ID));
	IAQDevice device(id, harness.HubLocatorRef(), /*is_emulated*/ false);

	BOOST_CHECK(device.SetSpaSwitchAssignment(1, 2, "Pool Heat") == Capabilities::ActuationResult::NotSupported);
}

BOOST_AUTO_TEST_CASE(SetSpaSwitchAssignment_InvalidArgs_IsInvalidValue)
{
	Test::MockReplayHarness harness;
	auto id = std::make_shared<JandyDeviceType>(JandyDeviceId(IAQ_UI_ID));
	IAQDevice device(id, harness.HubLocatorRef(), /*is_emulated*/ true);

	BOOST_CHECK(device.SetSpaSwitchAssignment(0, 1, "Pool Heat") == Capabilities::ActuationResult::InvalidValue);   // switch < 1
	BOOST_CHECK(device.SetSpaSwitchAssignment(1, 0, "Pool Heat") == Capabilities::ActuationResult::InvalidValue);   // button < 1
	BOOST_CHECK(device.SetSpaSwitchAssignment(1, 5, "Pool Heat") == Capabilities::ActuationResult::InvalidValue);   // button > 4
	BOOST_CHECK(device.SetSpaSwitchAssignment(1, 1, "")          == Capabilities::ActuationResult::InvalidValue);   // empty function
}

BOOST_AUTO_TEST_CASE(SetSpaSwitchAssignment_OffscreenRow_IsNotSupported)
{
	// Row 8 (switch 2 button 4) is not on-screen without an undecoded assignment-list scroll AND its
	// page-button index collides with the picker commit range -- so it is rejected (the OneTouch
	// writer still covers it). Higher switch numbers likewise fall through.
	Test::MockReplayHarness harness;
	auto id = std::make_shared<JandyDeviceType>(JandyDeviceId(IAQ_UI_ID));
	IAQDevice device(id, harness.HubLocatorRef(), /*is_emulated*/ true);

	BOOST_CHECK(device.SetSpaSwitchAssignment(2, 4, "Pool Heat") == Capabilities::ActuationResult::NotSupported);   // ordinal 8
	BOOST_CHECK(device.SetSpaSwitchAssignment(3, 1, "Pool Heat") == Capabilities::ActuationResult::NotSupported);   // ordinal 9
}

BOOST_AUTO_TEST_CASE(ControllerPriority_IsMedium_DirectChannel)
{
	Test::MockReplayHarness harness;
	auto id = std::make_shared<JandyDeviceType>(JandyDeviceId(IAQ_UI_ID));
	IAQDevice device(id, harness.HubLocatorRef(), /*is_emulated*/ true);

	BOOST_CHECK(device.ControllerPriority() == Capabilities::ActuationPriority::Medium);
}

BOOST_AUTO_TEST_CASE(AvailableFunctions_AreTheControllerPickerSet)
{
	// The iAQ surfaces the same canonical assignable-function set the UI offers as the strict chooser.
	Test::MockReplayHarness harness;
	auto id = std::make_shared<JandyDeviceType>(JandyDeviceId(IAQ_UI_ID));
	IAQDevice device(id, harness.HubLocatorRef(), /*is_emulated*/ true);

	const auto functions = device.AvailableFunctions();
	BOOST_REQUIRE(!functions.empty());
	BOOST_CHECK(std::find(functions.begin(), functions.end(), "Pool Light") != functions.end());
}

// Closed-loop wire-assertion: drive an emulated iAQ already on the 4-Function detail and assert it
// emits the decoded keypress bytes (captured off the JandyMessage_Ack send-publisher). Reproduces the
// two captured edits: 1:2 -> Pool Heat (visible at picker slot 3, no scroll) and 2:2 -> a function
// only reachable after a picker scroll (committed at slot 1).
BOOST_AUTO_TEST_CASE(SpaSwitchWrite_OnDetail_NoScroll_EmitsRowSelectThenCommit)
{
	Test::MockReplayHarness harness;
	auto id = std::make_shared<JandyDeviceType>(JandyDeviceId(IAQ_UI_ID));
	IAQDevice device(id, harness.HubLocatorRef(), /*is_emulated*/ true);

	std::vector<uint8_t> cmds;
	boost::signals2::scoped_connection conn = Messages::JandyMessage_Ack::GetPublisher()->connect(
		[&cmds](std::reference_wrapper<const Messages::JandyMessage_Ack> r)
		{
			if (r.get().Command() != 0x00) { cmds.push_back(r.get().Command()); }
		});

	BOOST_REQUIRE(device.SetSpaSwitchAssignment(1, 2, "Pool Heat") == Capabilities::ActuationResult::Accepted);

	// On the detail with the picker showing Pool Heat at slot 3.
	ReplayDetail(harness, "Pool Light", { {1, "Filter Pump"}, {2, "Spa"}, {3, "Pool Heat"}, {4, "Spa Heat"} });

	for (int i = 0; (i < 30) && (cmds.size() < 2); ++i) { harness.Replay({ Poll() }); }

	BOOST_REQUIRE_EQUAL(cmds.size(), 2u);
	BOOST_CHECK_EQUAL(static_cast<int>(cmds[0]), 0x17);   // row-select 1:2 (ordinal 2 -> 0x15 + 2)
	BOOST_CHECK_EQUAL(static_cast<int>(cmds[1]), 0x1f);   // commit picker slot 3 (0x1c + 3) = Pool Heat

	// The commit is the save: the master re-pushes 1:2 = Pool Heat; Verify then completes the goal.
	ReplayDetail(harness, "Pool Heat", { {1, "Filter Pump"}, {2, "Spa"}, {3, "Pool Heat"}, {4, "Spa Heat"} });
	for (int i = 0; i < 8; ++i) { harness.Replay({ Poll() }); }
	BOOST_CHECK_EQUAL(harness.DataHub()->SpaSwitchAssignment(1, 2).value_or(""), "Pool Heat");
	// Goal finished: no further commands are emitted on subsequent polls.
	const auto count_after = cmds.size();
	for (int i = 0; i < 5; ++i) { harness.Replay({ Poll() }); }
	BOOST_CHECK_EQUAL(cmds.size(), count_after);
}

BOOST_AUTO_TEST_CASE(SpaSwitchWrite_PickerScroll_EmitsRowSelectScrollCommit)
{
	Test::MockReplayHarness harness;
	auto id = std::make_shared<JandyDeviceType>(JandyDeviceId(IAQ_UI_ID));
	IAQDevice device(id, harness.HubLocatorRef(), /*is_emulated*/ true);

	std::vector<uint8_t> cmds;
	boost::signals2::scoped_connection conn = Messages::JandyMessage_Ack::GetPublisher()->connect(
		[&cmds](std::reference_wrapper<const Messages::JandyMessage_Ack> r)
		{
			if (r.get().Command() != 0x00) { cmds.push_back(r.get().Command()); }
		});

	BOOST_REQUIRE(device.SetSpaSwitchAssignment(2, 2, "Pool Light") == Capabilities::ActuationResult::Accepted);

	// Page A of the picker does NOT contain Pool Light -> the writer must scroll.
	ReplayDetail(harness, "Swim Jet", { {1, "Filter Pump"}, {2, "Spa"}, {3, "Pool Heat"}, {4, "Spa Heat"} });

	// Pump until the row-select + scroll are emitted.
	for (int i = 0; (i < 30) && (cmds.size() < 2); ++i) { harness.Replay({ Poll() }); }
	BOOST_REQUIRE_EQUAL(cmds.size(), 2u);
	BOOST_CHECK_EQUAL(static_cast<int>(cmds[0]), 0x1b);   // row-select 2:2 (ordinal 6 -> 0x15 + 6)
	BOOST_CHECK_EQUAL(static_cast<int>(cmds[1]), 0x15);   // scroll the picker

	// The master scrolls the picker to a page where Pool Light is at slot 1.
	ReplayPicker(harness, { {1, "Pool Light"}, {2, "Air Blower"}, {3, "Spillway"} });
	for (int i = 0; (i < 30) && (cmds.size() < 3); ++i) { harness.Replay({ Poll() }); }

	BOOST_REQUIRE_EQUAL(cmds.size(), 3u);
	BOOST_CHECK_EQUAL(static_cast<int>(cmds[2]), 0x1d);   // commit picker slot 1 (0x1c + 1) = Pool Light
}

BOOST_AUTO_TEST_SUITE_END()
