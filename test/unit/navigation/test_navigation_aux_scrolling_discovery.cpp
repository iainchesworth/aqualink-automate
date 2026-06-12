#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "navigation/menu_model.h"
#include "navigation/navigator.h"
#include "navigation/onetouch_menu_model.h"
#include "navigation/spider_engine.h"
#include "navigation/visit_policies.h"
#include "utility/screen_data_page.h"

#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/messages/jandy_message_ids.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/data_hub.h"

#include "support/unit_test_mockreplayharness.h"
#include "support/unit_test_protocolmessagebuilder.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Navigation;
using namespace AqualinkAutomate::Utility;

// =============================================================================
// OneTouch dynamic-scrolling aux-device discovery
//
// Resolves the TODO at onetouch_menu_model.cpp: power center A's aux outputs
// (Aux1-Aux7) sit on the first "Label Aux" screen, but the B/C/D power centers
// (Aux B1-D8) appear AFTER them and only become visible by scrolling the list.
// These tests prove the SpiderEngine + Navigator now discover and visit every
// aux row regardless of how far down the list it is, by:
//   * driving the REAL SpiderEngine/Navigator against a controller that models a
//     scrollable Label Aux list (so scrolling is genuinely exercised), and
//   * replaying synthetic OneTouch page frames through the FULL protocol/device
//     stack to confirm a scrolled-to B/C/D aux is actually registered in the
//     DataHub.
// =============================================================================

namespace
{

// -----------------------------------------------------------------------------
// ScrollableLabelAuxController
//
// A minimal but faithful OneTouch controller simulation focused on the Label Aux
// area.  It owns a flat list of aux rows ("Aux1".."Aux7","Aux B1".. etc.) that is
// TALLER than the visible window, renders an 8-row window with a "^^ More vv"
// scroll indicator, and reacts to LineUp/LineDown/PageDown by moving the cursor
// and scrolling the window exactly as the panel does.  Selecting an aux row opens
// that aux's Label Aux detail page (a distinct LabelAux instance); Back returns
// to the list.  All other pages are served from a small fixed registry so the
// spider can reach the Label Aux list from the System page.
// -----------------------------------------------------------------------------
class ScrollableLabelAuxController
{
public:
	static constexpr uint8_t LINES = 12;
	static constexpr uint8_t LIST_FIRST_ROW = 2;   // line 0 = title, line 1 = blank.
	static constexpr uint8_t LIST_LAST_ROW = 9;    // line 10 = "^^ More vv".
	static constexpr uint8_t WINDOW_ROWS = (LIST_LAST_ROW - LIST_FIRST_ROW) + 1; // 8 rows visible.

	ScrollableLabelAuxController(const MenuModel& model, std::vector<std::string> aux_rows)
		: m_Model(model)
		, m_AuxRows(std::move(aux_rows))
	{
	}

	PageId CurrentPage() const { return m_CurrentPage; }
	uint8_t Cursor() const { return m_Cursor; }
	const std::string& CurrentAuxLabel() const { return m_CurrentAuxLabel; }

	ScreenDataPage Content() const
	{
		ScreenDataPage page(LINES);

		switch (m_CurrentPage)
		{
		case PageId::System:
			page[9].Text = "Equipment ON/OFF";
			page[10].Text = "OneTouch ON/OFF";
			page[11].Text = "Menu/Help";
			break;

		case PageId::MenuHelp:
			page[0].Text = "Menu";
			page[1].Text = "Help";
			page[10].Text = "System Setup";
			break;

		case PageId::SystemSetup:
			page[0].Text = "System Setup";
			page[2].Text = "Label Aux";
			break;

		case PageId::LabelAuxList:
		{
			page[0].Text = "   Label Aux";
			for (uint8_t row = 0; row < WINDOW_ROWS; ++row)
			{
				const std::size_t aux_index = m_ScrollOffset + row;
				if (aux_index < m_AuxRows.size())
				{
					page[LIST_FIRST_ROW + row].Text = m_AuxRows[aux_index] + "           >";
				}
			}
			page[10].Text = "   ^^ More vv";
			break;
		}

		case PageId::LabelAux:
			// Detail page for the aux selected from the list.
			page[0].Text = "   Label " + StripSpaces(m_CurrentAuxLabel);
			page[2].Text = " Current Label ";
			page[3].Text = "      " + m_CurrentAuxLabel;
			page[5].Text = "General Labels >";
			page[6].Text = "Light   Labels >";
			page[7].Text = "Wtrfall Labels >";
			break;

		default:
		{
			// Serve any other page from its detectors so DetectPage recognises it.
			const MenuPage* mp = m_Model.GetPage(m_CurrentPage);
			if (mp)
			{
				for (const auto& d : mp->detectors)
				{
					if (d.line < page.Size())
					{
						page[d.line].Text = d.pattern;
					}
				}
			}
			break;
		}
		}

		return page;
	}

	void Execute(NavKeyCommand cmd)
	{
		switch (m_CurrentPage)
		{
		case PageId::LabelAuxList:
			ExecuteOnList(cmd);
			break;

		case PageId::LabelAux:
			if (cmd == NavKeyCommand::Back)
			{
				m_CurrentPage = PageId::LabelAuxList;
				// Cursor returns to the row the aux was selected from.
				m_Cursor = static_cast<uint8_t>(LIST_FIRST_ROW + (m_SelectedAuxIndex - m_ScrollOffset));
			}
			break;

		default:
			ExecuteGeneric(cmd);
			break;
		}
	}

private:
	static std::string StripSpaces(std::string s)
	{
		s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
		return s;
	}

	void ExecuteGeneric(NavKeyCommand cmd)
	{
		const MenuPage* page = m_Model.GetPage(m_CurrentPage);
		if (!page) { return; }

		switch (cmd)
		{
		case NavKeyCommand::Select:
			for (const auto& edge : page->edges)
			{
				if (edge.trigger == EdgeTrigger::Select && edge.trigger_line == m_Cursor)
				{
					m_CurrentPage = edge.target;
					m_Cursor = 0;
					return;
				}
			}
			// Fall back to label match for robustness against shifted layouts.
			for (const auto& edge : page->edges)
			{
				if (edge.trigger == EdgeTrigger::Select && !edge.label.empty())
				{
					auto content = Content();
					if (m_Cursor < content.Size() && Trim(content[m_Cursor].Text).starts_with(edge.label))
					{
						m_CurrentPage = edge.target;
						m_Cursor = 0;
						return;
					}
				}
			}
			break;

		case NavKeyCommand::Back:
			for (const auto& edge : page->edges)
			{
				if (edge.trigger == EdgeTrigger::Back)
				{
					m_CurrentPage = edge.target;
					m_Cursor = 0;
					return;
				}
			}
			break;

		case NavKeyCommand::LineDown:
			if (m_Cursor + 1 < LINES) { ++m_Cursor; }
			break;
		case NavKeyCommand::LineUp:
			if (m_Cursor > 0) { --m_Cursor; }
			break;
		default:
			break;
		}
	}

	void ExecuteOnList(NavKeyCommand cmd)
	{
		const std::size_t max_offset = (m_AuxRows.size() > WINDOW_ROWS)
			? (m_AuxRows.size() - WINDOW_ROWS) : 0;

		switch (cmd)
		{
		case NavKeyCommand::Select:
		{
			const std::size_t aux_index = m_ScrollOffset + (m_Cursor - LIST_FIRST_ROW);
			if (m_Cursor >= LIST_FIRST_ROW && aux_index < m_AuxRows.size())
			{
				m_SelectedAuxIndex = aux_index;
				m_CurrentAuxLabel = m_AuxRows[aux_index];
				m_CurrentPage = PageId::LabelAux;
				m_Cursor = 0;
			}
			break;
		}

		case NavKeyCommand::LineDown:
			if (m_Cursor < LIST_LAST_ROW)
			{
				++m_Cursor;
			}
			else if (m_ScrollOffset < max_offset)
			{
				// Cursor at bottom of window: scroll the list down one row.
				++m_ScrollOffset;
			}
			break;

		case NavKeyCommand::LineUp:
			if (m_Cursor > LIST_FIRST_ROW)
			{
				--m_Cursor;
			}
			else if (m_ScrollOffset > 0)
			{
				--m_ScrollOffset;
			}
			break;

		case NavKeyCommand::PageDown_Or_Select1:
			// Page scroll: advance the window by a full page, clamping at the end.
			m_ScrollOffset = std::min(m_ScrollOffset + WINDOW_ROWS, max_offset);
			m_Cursor = LIST_FIRST_ROW;
			break;

		case NavKeyCommand::PageUp_Or_Select3:
			m_ScrollOffset = (m_ScrollOffset > WINDOW_ROWS) ? (m_ScrollOffset - WINDOW_ROWS) : 0;
			m_Cursor = LIST_FIRST_ROW;
			break;

		default:
			break;
		}
	}

	static std::string Trim(const std::string& s)
	{
		auto start = s.find_first_not_of(" \t");
		if (start == std::string::npos) { return {}; }
		auto end = s.find_last_not_of(" \t");
		return s.substr(start, end - start + 1);
	}

private:
	const MenuModel& m_Model;
	std::vector<std::string> m_AuxRows;

	PageId m_CurrentPage{ PageId::System };
	uint8_t m_Cursor{ 0 };
	std::size_t m_ScrollOffset{ 0 };
	std::size_t m_SelectedAuxIndex{ 0 };
	std::string m_CurrentAuxLabel;
};

// How many JandyMessage_Status updates a command consumes (mirrors the Navigator
// pacing: page transitions = 2, cursor/scroll moves = 1).
int StatusMessageCount(NavKeyCommand cmd)
{
	switch (cmd)
	{
	case NavKeyCommand::Select:
	case NavKeyCommand::Back:
		return 2;
	default:
		return 1;
	}
}

// Drive a full crawl that captures the label of every LabelAux instance the
// spider reaches.  Returns the set of captured aux labels.
std::set<std::string> RunCrawlAndCollectAuxLabels(
	const MenuModel& model,
	ScrollableLabelAuxController& controller,
	SpiderEngine::State& final_state,
	bool& label_aux_marked_visited)
{
	Navigator nav(model);
	SpiderEngine engine(model, nav);

	std::set<std::string> captured_aux_labels;

	auto policy = std::make_unique<TargetedVisitPolicy>(
		std::set<PageId>{ PageId::LabelAux },
		[&](PageId page, const ScreenDataPage& content)
		{
			if (page == PageId::LabelAux)
			{
				// Line 3 carries the aux's current label on the detail page.
				auto start = content[3].Text.find_first_not_of(" \t");
				if (start != std::string::npos)
				{
					auto end = content[3].Text.find_last_not_of(" \t");
					captured_aux_labels.insert(content[3].Text.substr(start, end - start + 1));
				}
			}
		},
		nullptr);
	engine.StartCrawl(std::move(policy));

	const uint32_t MAX_ITERATIONS = 50000;
	uint32_t iterations = 0;
	while (engine.GetState() != SpiderEngine::State::Complete &&
	       engine.GetState() != SpiderEngine::State::Failed &&
	       iterations < MAX_ITERATIONS)
	{
		++iterations;
		auto cmd = engine.ProcessStep(controller.Content(), controller.Cursor());
		if (cmd.has_value())
		{
			for (int i = 0; i < StatusMessageCount(cmd.value()); ++i)
			{
				nav.OnStatusMessageReceived();
			}
			controller.Execute(cmd.value());
		}
	}

	final_state = engine.GetState();
	// Page-level accounting: the multi-instance LabelAux page must be marked
	// fully visited once every aux instance has been captured or benignly
	// skipped (regression guard for the "last instance absent" accounting path).
	label_aux_marked_visited = engine.GetVisitedPages().count(PageId::LabelAux) > 0;
	return captured_aux_labels;
}

} // namespace

// =============================================================================
// SpiderEngine-driven scrolling discovery
// =============================================================================
BOOST_AUTO_TEST_SUITE(AuxScrollingDiscovery_TestSuite)

// Power center A only (Aux1-Aux7) — regression: the previously-working primary
// auxes must still be discovered after the dynamic-scrolling changes.
BOOST_AUTO_TEST_CASE(TestDiscoversPowerCenterAOnly)
{
	auto model = CreateOneTouchMenuModel();
	std::vector<std::string> rows = { "Aux1", "Aux2", "Aux3", "Aux4", "Aux5", "Aux6", "Aux7" };
	ScrollableLabelAuxController controller(model, rows);

	SpiderEngine::State state{};
	bool label_aux_visited = false;
	auto captured = RunCrawlAndCollectAuxLabels(model, controller, state, label_aux_visited);

	BOOST_CHECK(state == SpiderEngine::State::Complete);
	BOOST_CHECK(label_aux_visited);
	for (const auto& label : rows)
	{
		BOOST_CHECK_MESSAGE(captured.count(label) > 0,
			"Power center A aux '" << label << "' was not discovered");
	}
	// No phantom B/C/D auxes should have been captured.
	BOOST_CHECK_EQUAL(captured.size(), rows.size());
}

// Power centers A + B: the B-series sits just below the fold and must be reached
// by scrolling the list.
BOOST_AUTO_TEST_CASE(TestDiscoversPowerCenterBByScrolling)
{
	auto model = CreateOneTouchMenuModel();
	std::vector<std::string> rows = {
		"Aux1", "Aux2", "Aux3", "Aux4", "Aux5", "Aux6", "Aux7",
		"Aux B1", "Aux B2", "Aux B3", "Aux B4", "Aux B5", "Aux B6", "Aux B7", "Aux B8"
	};
	ScrollableLabelAuxController controller(model, rows);

	SpiderEngine::State state{};
	bool label_aux_visited = false;
	auto captured = RunCrawlAndCollectAuxLabels(model, controller, state, label_aux_visited);

	BOOST_CHECK(state == SpiderEngine::State::Complete);
	BOOST_CHECK(label_aux_visited);

	// Every B-series aux must be discovered (these are below the first screen).
	for (int n = 1; n <= 8; ++n)
	{
		const std::string label = "Aux B" + std::to_string(n);
		BOOST_CHECK_MESSAGE(captured.count(label) > 0,
			"Power center B aux '" << label << "' was not discovered by scrolling");
	}
	// And the A-series are still discovered.
	BOOST_CHECK(captured.count("Aux1") > 0);
	BOOST_CHECK(captured.count("Aux7") > 0);
	BOOST_CHECK_EQUAL(captured.size(), rows.size());
}

// Full house: power centers A + B + C + D (all 31 aux outputs). The C and D
// series are far below the fold and require repeated scrolling to discover.
BOOST_AUTO_TEST_CASE(TestDiscoversPowerCentersBCDByScrolling)
{
	auto model = CreateOneTouchMenuModel();
	std::vector<std::string> rows;
	for (int n = 1; n <= 7; ++n) { rows.push_back("Aux" + std::to_string(n)); }
	for (char pc : { 'B', 'C', 'D' })
	{
		for (int n = 1; n <= 8; ++n)
		{
			rows.push_back(std::string("Aux ") + pc + std::to_string(n));
		}
	}
	BOOST_REQUIRE_EQUAL(rows.size(), 31u);

	ScrollableLabelAuxController controller(model, rows);

	SpiderEngine::State state{};
	bool label_aux_visited = false;
	auto captured = RunCrawlAndCollectAuxLabels(model, controller, state, label_aux_visited);

	BOOST_CHECK(state == SpiderEngine::State::Complete);
	BOOST_CHECK(label_aux_visited);

	for (char pc : { 'B', 'C', 'D' })
	{
		for (int n = 1; n <= 8; ++n)
		{
			const std::string label = std::string("Aux ") + pc + std::to_string(n);
			BOOST_CHECK_MESSAGE(captured.count(label) > 0,
				"Aux '" << label << "' on power center " << pc << " was not discovered by scrolling");
		}
	}
	// All 31 aux outputs (A1-7, B1-8, C1-8, D1-8) discovered exactly once each.
	BOOST_CHECK_EQUAL(captured.size(), 31u);
}

// A partially-populated controller (A + a few B auxes) — the model declares 31
// possible aux edges but only the installed ones exist on-screen.  The missing
// rows must be benign per-edge skips, NOT a crawl failure.
BOOST_AUTO_TEST_CASE(TestMissingAuxesAreBenignSkipsNotCrawlFailure)
{
	auto model = CreateOneTouchMenuModel();
	std::vector<std::string> rows = {
		"Aux1", "Aux2", "Aux3", "Aux4", "Aux5", "Aux6", "Aux7",
		"Aux B1", "Aux B2", "Aux B3"   // B4-B8, all of C and D are absent.
	};
	ScrollableLabelAuxController controller(model, rows);

	SpiderEngine::State state{};
	bool label_aux_visited = false;
	auto captured = RunCrawlAndCollectAuxLabels(model, controller, state, label_aux_visited);

	// Despite 21 declared-but-absent aux edges, the crawl must still complete.
	BOOST_CHECK_MESSAGE(state == SpiderEngine::State::Complete,
		"Crawl did not complete - absent aux edges must be benign skips, not failures");

	// The multi-instance page must be counted as visited even though its last
	// declared instance (Aux D8) was absent and benignly skipped.
	BOOST_CHECK(label_aux_visited);

	// Exactly the installed auxes are discovered; absent ones are not invented.
	BOOST_CHECK_EQUAL(captured.size(), rows.size());
	BOOST_CHECK(captured.count("Aux B3") > 0);
	BOOST_CHECK(captured.count("Aux B4") == 0);
	BOOST_CHECK(captured.count("Aux C1") == 0);
	BOOST_CHECK(captured.count("Aux D8") == 0);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// End-to-end decode: a scrolled-to B/C/D aux's Label Aux page, replayed through
// the FULL protocol + OneTouchDevice stack, registers the aux in the DataHub.
//
// This proves that once the navigator scrolls to a B/C/D aux and opens its
// detail page, the resulting wire frames actually produce a registered device
// (PageProcessor_LabelAux), closing the loop from navigation to device state.
// =============================================================================
BOOST_AUTO_TEST_SUITE(AuxScrollingDiscovery_DeviceRegistration_TestSuite)

namespace
{
	constexpr uint8_t ONETOUCH_TEST_DEVICE_ID = 0x00; // dest 0x00 => not filtered out.
	const uint8_t CMD_MESSAGE_LONG = static_cast<uint8_t>(Messages::JandyMessageIds::MessageLong);

	// Replay one full "Label Aux" detail screen: a MessageLong per line followed
	// by a Status frame that finalises the page and triggers PageProcessor_LabelAux.
	void ReplayLabelAuxDetailPage(Test::MockReplayHarness& harness,
		const std::vector<std::pair<uint8_t, std::string>>& lines)
	{
		std::vector<std::vector<uint8_t>> frames;
		for (const auto& [line_id, text] : lines)
		{
			std::vector<uint8_t> payload;
			payload.push_back(line_id);
			for (char c : text) { payload.push_back(static_cast<uint8_t>(c)); }
			frames.push_back(Test::MessageBuilder::CreateValidChecksummedMessage(
				ONETOUCH_TEST_DEVICE_ID, CMD_MESSAGE_LONG, payload));
		}
		// Status (cmd 0x02) marks the screen update complete and triggers the
		// page processor.  A real status packet carries a 5-byte flags payload;
		// the generator rejects a short frame, so send the full 5 bytes.
		frames.push_back(Test::MessageBuilder::CreateValidChecksummedMessage(
			ONETOUCH_TEST_DEVICE_ID, static_cast<uint8_t>(Messages::JandyMessageIds::Status),
			{ 0x00, 0x00, 0x00, 0x00, 0x00 }));

		harness.Replay(frames);
	}

	bool DataHubHasAuxLabel(Test::MockReplayHarness& harness, const std::string& label)
	{
		for (const auto& dev : harness.DataHub()->Auxillaries())
		{
			if (dev == nullptr) { continue; }
			auto lbl = dev->AuxillaryTraits.TryGet(Kernel::AuxillaryTraitsTypes::LabelTrait{});
			if (lbl.has_value() && lbl.value() == label)
			{
				return true;
			}
		}
		return false;
	}
}

BOOST_AUTO_TEST_CASE(Replay_PowerCenterBAuxDetailPage_RegistersDevice)
{
	Test::MockReplayHarness harness;

	// Non-emulated OneTouch device: it observes screens passively and runs the
	// PageProcessors (the navigator/spider does the scrolling in production; here
	// we replay the page the scroll would have landed on).
	auto device_id = std::make_shared<Devices::JandyDeviceType>(Devices::JandyDeviceId(ONETOUCH_TEST_DEVICE_ID));
	auto onetouch = std::make_shared<Devices::OneTouchDevice>(device_id, harness.HubLocatorRef(), false);

	BOOST_REQUIRE(harness.DataHub()->Auxillaries().empty());

	// The Label Aux detail page for "Aux B1" with a user-assigned custom label.
	// The title line carries the default aux id ("Label Aux B1"); the regex in
	// PageProcessor_LabelAux maps it to JandyAuxillaryIds::Aux_B1.
	ReplayLabelAuxDetailPage(harness, {
		{ 0x0, "  Label Aux B1  " },
		{ 0x2, " Current Label  " },
		{ 0x3, "   Swim Jet     " },
		{ 0x5, "General Labels >" },
		{ 0x6, "Light   Labels >" },
		{ 0x7, "Wtrfall Labels >" },
	});

	BOOST_CHECK_EQUAL(harness.StatisticsHub()->MessageErrors.ChecksumFailures, 0u);

	// The B-series aux must now exist in the DataHub carrying its custom label.
	BOOST_CHECK_MESSAGE(DataHubHasAuxLabel(harness, "Swim Jet"),
		"Power center B aux (Aux B1) was not registered with its custom label after replay");

	// Keep the device alive until after the assertions, then release it before
	// the harness tears down its hubs.
	onetouch.reset();
}

BOOST_AUTO_TEST_CASE(Replay_PowerCenterDAuxDetailPage_RegistersDevice)
{
	Test::MockReplayHarness harness;

	auto device_id = std::make_shared<Devices::JandyDeviceType>(Devices::JandyDeviceId(ONETOUCH_TEST_DEVICE_ID));
	auto onetouch = std::make_shared<Devices::OneTouchDevice>(device_id, harness.HubLocatorRef(), false);

	// Label Aux detail page for "Aux D8" (the very last, deepest aux output).
	ReplayLabelAuxDetailPage(harness, {
		{ 0x0, "  Label Aux D8  " },
		{ 0x2, " Current Label  " },
		{ 0x3, "   Deck Jets    " },
		{ 0x5, "General Labels >" },
		{ 0x6, "Light   Labels >" },
		{ 0x7, "Wtrfall Labels >" },
	});

	BOOST_CHECK_EQUAL(harness.StatisticsHub()->MessageErrors.ChecksumFailures, 0u);
	BOOST_CHECK_MESSAGE(DataHubHasAuxLabel(harness, "Deck Jets"),
		"Power center D aux (Aux D8) was not registered with its custom label after replay");

	onetouch.reset();
}

BOOST_AUTO_TEST_SUITE_END()
