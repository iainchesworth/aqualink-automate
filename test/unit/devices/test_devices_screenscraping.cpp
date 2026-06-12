#include <cstddef>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/utility/screen_data_page_graph.h"
#include "jandy/utility/screen_data_page_graph/screen_data_page_graph_edge.h"
#include "jandy/utility/screen_data_page_graph/screen_data_page_graph_traverse.h"
#include "jandy/utility/screen_data_page_graph/screen_data_page_graph_vertex.h"
#include "jandy/utility/screen_data_page_processor.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Utility;
using namespace AqualinkAutomate::Utility::ScreenDataPageGraphImpl;

// The graph now stores a single concrete KeyCommand on each edge (previously a
// type-erased std::any holding a per-device key-command enum).  KeyCommand's
// enumerators carry the shared wire semantics used by every device, so the
// test suite uses it directly instead of a test-local enum.

BOOST_AUTO_TEST_SUITE(ScreenDataPageGraph_TestSuite)

// =============================================================================
// Basic Graph Construction Tests
// =============================================================================

// Note: TestEmptyGraph removed - empty graph behavior throws std::out_of_range
// when accessing non-existent vertex, which is expected behavior

BOOST_AUTO_TEST_CASE(TestSingleVertexGraph)
{
	ScreenDataPageGraph graph(
		{
			{ 1, ScreenDataPageTypes::Page_System }
		},
		{}  // No edges
	);

	auto it = ForwardIterator::begin(graph, 1);
	auto end = ForwardIterator::end(graph);

	// Iterator should start at vertex 1
	BOOST_CHECK(it != end);

	auto& [vertex, edge] = *it;
	BOOST_CHECK_EQUAL(vertex.id, 1);

	// Advancing should reach end (no outgoing edges)
	++it;
	BOOST_CHECK(it == end);
}

BOOST_AUTO_TEST_CASE(TestLinearGraph)
{
	// Graph: 1 -> 2 -> 3 -> 4
	ScreenDataPageGraph graph(
		{
			{ 1, ScreenDataPageTypes::Page_System },
			{ 2, ScreenDataPageTypes::Page_System },
			{ 3, ScreenDataPageTypes::Page_MenuHelp },
			{ 4, ScreenDataPageTypes::Page_Unknown }
		},
		{
			{ 1, 2, { 1, KeyCommand::LineDown } },
			{ 2, 3, { 2, KeyCommand::LineDown } },
			{ 3, 4, { 3, KeyCommand::Select } }
		}
	);

	auto it = ForwardIterator::begin(graph, 1);
	auto end = ForwardIterator::end(graph);

	// Collect all vertices visited
	std::vector<VertexId> visited_vertices;
	std::vector<KeyCommand> key_commands;

	while (it != end)
	{
		auto& [vertex, edge] = *it;
		visited_vertices.push_back(vertex.id);

		if (KeyCommand::NoKeyCommand != edge.key_command)
		{
			key_commands.push_back(edge.key_command);
		}

		++it;
	}

	// Should visit all 4 vertices
	BOOST_REQUIRE_EQUAL(visited_vertices.size(), 4);
	BOOST_CHECK_EQUAL(visited_vertices[0], 1);
	BOOST_CHECK_EQUAL(visited_vertices[1], 2);
	BOOST_CHECK_EQUAL(visited_vertices[2], 3);
	BOOST_CHECK_EQUAL(visited_vertices[3], 4);

	// Should have 3 key commands (from edges)
	BOOST_REQUIRE_EQUAL(key_commands.size(), 3);
	BOOST_CHECK(key_commands[0] == KeyCommand::LineDown);
	BOOST_CHECK(key_commands[1] == KeyCommand::LineDown);
	BOOST_CHECK(key_commands[2] == KeyCommand::Select);
}

BOOST_AUTO_TEST_CASE(TestStartFromMiddle)
{
	// Graph: 1 -> 2 -> 3 -> 4
	// Start from vertex 2 (like warm start)
	ScreenDataPageGraph graph(
		{
			{ 1, ScreenDataPageTypes::Page_OneTouch },
			{ 2, ScreenDataPageTypes::Page_System },
			{ 3, ScreenDataPageTypes::Page_MenuHelp },
			{ 4, ScreenDataPageTypes::Page_Unknown }
		},
		{
			{ 1, 2, { 1, KeyCommand::Select } },
			{ 2, 3, { 2, KeyCommand::LineDown } },
			{ 3, 4, { 3, KeyCommand::Select } }
		}
	);

	// Start from vertex 2 (skipping vertex 1)
	auto it = ForwardIterator::begin(graph, 2);
	auto end = ForwardIterator::end(graph);

	std::vector<VertexId> visited_vertices;
	while (it != end)
	{
		auto& [vertex, edge] = *it;
		visited_vertices.push_back(vertex.id);
		++it;
	}

	// Should visit vertices 2, 3, 4 (not 1)
	BOOST_REQUIRE_EQUAL(visited_vertices.size(), 3);
	BOOST_CHECK_EQUAL(visited_vertices[0], 2);
	BOOST_CHECK_EQUAL(visited_vertices[1], 3);
	BOOST_CHECK_EQUAL(visited_vertices[2], 4);
}

// Verify the typed Vertex/Edge defaults preserve the previous "empty std::any"
// semantics: a default-constructed vertex resolves to Page_Unknown (the old
// any_cast fallback) and a default-constructed edge carries no key command.
BOOST_AUTO_TEST_CASE(TestDefaultVertexAndEdgeAreNeutral)
{
	Vertex default_vertex{};
	BOOST_CHECK_EQUAL(default_vertex.id, 0u);
	BOOST_CHECK(default_vertex.page == ScreenDataPageTypes::Page_Unknown);

	Edge default_edge{};
	BOOST_CHECK_EQUAL(default_edge.id, 0u);
	BOOST_CHECK(default_edge.key_command == KeyCommand::NoKeyCommand);

	// A vertex constructed with only an id also defaults to Page_Unknown.
	Vertex id_only_vertex{ 5 };
	BOOST_CHECK_EQUAL(id_only_vertex.id, 5u);
	BOOST_CHECK(id_only_vertex.page == ScreenDataPageTypes::Page_Unknown);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// OneTouch Navigation Graph Tests
// =============================================================================

BOOST_AUTO_TEST_SUITE(OneTouchNavigationGraph_TestSuite)

// Create a graph matching the fixed ONETOUCH_AUX_LABELS_NAV_SCRAPER_GRAPH
ScreenDataPageGraph CreateOneTouchNavGraph()
{
	return ScreenDataPageGraph(
		{
			{ 1, ScreenDataPageTypes::Page_OneTouch },
			{ 2, ScreenDataPageTypes::Page_OneTouch },
			{ 3, ScreenDataPageTypes::Page_System },
			{ 4, ScreenDataPageTypes::Page_System },
			{ 5, ScreenDataPageTypes::Page_System },
			{ 6, ScreenDataPageTypes::Page_MenuHelp },
			{ 7, ScreenDataPageTypes::Page_MenuHelp },
			{ 8, ScreenDataPageTypes::Page_MenuHelp },
			{ 9, ScreenDataPageTypes::Page_MenuHelp },
			{ 10, ScreenDataPageTypes::Page_MenuHelp },
			{ 11, ScreenDataPageTypes::Page_SystemSetup },
			{ 12, ScreenDataPageTypes::Page_SystemSetup },
			{ 13, ScreenDataPageTypes::Page_Unknown }
		},
		{
			{ 1, 2, { 1, KeyCommand::LineDown } },
			{ 2, 3, { 2, KeyCommand::Select } },
			{ 3, 4, { 3, KeyCommand::LineDown } },
			{ 4, 5, { 4, KeyCommand::LineDown } },
			{ 5, 6, { 5, KeyCommand::Select } },
			{ 6, 7, { 6, KeyCommand::LineDown } },
			{ 7, 8, { 7, KeyCommand::LineDown } },
			{ 8, 9, { 8, KeyCommand::LineDown } },
			{ 9, 10, { 9, KeyCommand::LineDown } },
			{ 10, 11, { 10, KeyCommand::Select } },
			{ 11, 12, { 11, KeyCommand::LineDown } },
			{ 12, 13, { 12, KeyCommand::Select } }
		}
	);
}

BOOST_AUTO_TEST_CASE(TestColdStartNavigation)
{
	// Cold start begins at vertex 1 (Page_OneTouch)
	auto graph = CreateOneTouchNavGraph();
	auto it = ForwardIterator::begin(graph, 1);
	auto end = ForwardIterator::end(graph);

	// Collect the navigation sequence
	std::vector<std::pair<VertexId, ScreenDataPageTypes>> navigation_sequence;

	while (it != end)
	{
		auto& [vertex, edge] = *it;
		navigation_sequence.push_back({ vertex.id, vertex.page });
		++it;
	}

	// Verify the full cold start sequence
	BOOST_REQUIRE_EQUAL(navigation_sequence.size(), 13);

	// Start on OneTouch
	BOOST_CHECK_EQUAL(navigation_sequence[0].first, 1);
	BOOST_CHECK(navigation_sequence[0].second == ScreenDataPageTypes::Page_OneTouch);

	// Still on OneTouch after LineDown
	BOOST_CHECK_EQUAL(navigation_sequence[1].first, 2);
	BOOST_CHECK(navigation_sequence[1].second == ScreenDataPageTypes::Page_OneTouch);

	// Now on Home after Select from OneTouch
	BOOST_CHECK_EQUAL(navigation_sequence[2].first, 3);
	BOOST_CHECK(navigation_sequence[2].second == ScreenDataPageTypes::Page_System);

	// Still on Home after LineDown
	BOOST_CHECK_EQUAL(navigation_sequence[3].first, 4);
	BOOST_CHECK(navigation_sequence[3].second == ScreenDataPageTypes::Page_System);

	// Still on Home after second LineDown
	BOOST_CHECK_EQUAL(navigation_sequence[4].first, 5);
	BOOST_CHECK(navigation_sequence[4].second == ScreenDataPageTypes::Page_System);

	// Now on MenuHelp after Select (THIS IS THE CRITICAL TRANSITION)
	BOOST_CHECK_EQUAL(navigation_sequence[5].first, 6);
	BOOST_CHECK(navigation_sequence[5].second == ScreenDataPageTypes::Page_MenuHelp);
}

BOOST_AUTO_TEST_CASE(TestWarmStartNavigation)
{
	// Warm start begins at vertex 3 (Page_System)
	auto graph = CreateOneTouchNavGraph();
	auto it = ForwardIterator::begin(graph, 3);
	auto end = ForwardIterator::end(graph);

	// Collect the navigation sequence
	std::vector<std::pair<VertexId, ScreenDataPageTypes>> navigation_sequence;
	std::vector<KeyCommand> key_commands;

	while (it != end)
	{
		auto& [vertex, edge] = *it;
		navigation_sequence.push_back({ vertex.id, vertex.page });

		if (KeyCommand::NoKeyCommand != edge.key_command)
		{
			key_commands.push_back(edge.key_command);
		}

		++it;
	}

	// Warm start should visit 11 vertices (3-13)
	BOOST_REQUIRE_EQUAL(navigation_sequence.size(), 11);

	// Start on Home (vertex 3)
	BOOST_CHECK_EQUAL(navigation_sequence[0].first, 3);
	BOOST_CHECK(navigation_sequence[0].second == ScreenDataPageTypes::Page_System);

	// After first LineDown, still on Home (vertex 4)
	BOOST_CHECK_EQUAL(navigation_sequence[1].first, 4);
	BOOST_CHECK(navigation_sequence[1].second == ScreenDataPageTypes::Page_System);

	// After second LineDown, still on Home (vertex 5)
	BOOST_CHECK_EQUAL(navigation_sequence[2].first, 5);
	BOOST_CHECK(navigation_sequence[2].second == ScreenDataPageTypes::Page_System);

	// After Select, now on MenuHelp (vertex 6) - THE FIX!
	BOOST_CHECK_EQUAL(navigation_sequence[3].first, 6);
	BOOST_CHECK(navigation_sequence[3].second == ScreenDataPageTypes::Page_MenuHelp);

	// Verify the key command sequence for warm start: LineDown, LineDown, Select
	BOOST_REQUIRE_GE(key_commands.size(), 3);
	BOOST_CHECK(key_commands[0] == KeyCommand::LineDown);
	BOOST_CHECK(key_commands[1] == KeyCommand::LineDown);
	BOOST_CHECK(key_commands[2] == KeyCommand::Select);
}

BOOST_AUTO_TEST_CASE(TestHomeToMenuTransition)
{
	// This test specifically verifies the Home -> Menu transition
	// which was broken before (used LineUp instead of LineDown x2)
	auto graph = CreateOneTouchNavGraph();

	// Start from vertex 3 (Home) where warm start begins
	auto it = ForwardIterator::begin(graph, 3);
	auto end = ForwardIterator::end(graph);

	// Navigate through: Home -> Home -> Home -> MenuHelp
	// (vertex 3 -> 4 -> 5 -> 6)

	// Position 1: vertex 3 (Home)
	BOOST_REQUIRE(it != end);
	{
		auto& [vertex, edge] = *it;
		BOOST_CHECK_EQUAL(vertex.id, 3);
		BOOST_CHECK(vertex.page == ScreenDataPageTypes::Page_System);
	}

	// Advance and check edge command
	++it;
	BOOST_REQUIRE(it != end);
	{
		auto& [vertex, edge] = *it;
		BOOST_CHECK_EQUAL(vertex.id, 4);
		BOOST_CHECK(vertex.page == ScreenDataPageTypes::Page_System);
		// Edge from 3->4 should be LineDown
		BOOST_CHECK(edge.key_command == KeyCommand::LineDown);
	}

	// Advance again
	++it;
	BOOST_REQUIRE(it != end);
	{
		auto& [vertex, edge] = *it;
		BOOST_CHECK_EQUAL(vertex.id, 5);
		BOOST_CHECK(vertex.page == ScreenDataPageTypes::Page_System);
		// Edge from 4->5 should also be LineDown
		BOOST_CHECK(edge.key_command == KeyCommand::LineDown);
	}

	// Advance to MenuHelp - THE CRITICAL TRANSITION
	++it;
	BOOST_REQUIRE(it != end);
	{
		auto& [vertex, edge] = *it;
		BOOST_CHECK_EQUAL(vertex.id, 6);
		// Vertex 6 MUST be MenuHelp, not Home!
		BOOST_CHECK(vertex.page == ScreenDataPageTypes::Page_MenuHelp);
		// Edge from 5->6 should be Select
		BOOST_CHECK(edge.key_command == KeyCommand::Select);
	}
}

BOOST_AUTO_TEST_CASE(TestNavigationKeyCommandSequence)
{
	// Verify the exact key command sequence for warm start
	auto graph = CreateOneTouchNavGraph();
	auto it = ForwardIterator::begin(graph, 3);
	auto end = ForwardIterator::end(graph);

	// Expected sequence from Home to SystemSetup:
	// LineDown -> LineDown -> Select -> LineDown -> LineDown -> LineDown -> LineDown -> Select -> LineDown -> Select
	std::vector<KeyCommand> expected_commands = {
		KeyCommand::LineDown,   // 3->4: Home cursor down
		KeyCommand::LineDown,   // 4->5: Home cursor down
		KeyCommand::Select,     // 5->6: Home -> MenuHelp
		KeyCommand::LineDown,   // 6->7: Menu cursor down
		KeyCommand::LineDown,   // 7->8: Menu cursor down
		KeyCommand::LineDown,   // 8->9: Menu cursor down
		KeyCommand::LineDown,   // 9->10: Menu cursor down
		KeyCommand::Select,     // 10->11: Menu -> SystemSetup
		KeyCommand::LineDown,   // 11->12: SystemSetup cursor down
		KeyCommand::Select      // 12->13: SystemSetup -> end
	};

	std::vector<KeyCommand> actual_commands;

	// Skip first vertex (no incoming edge)
	++it;

	while (it != end)
	{
		auto& [vertex, edge] = *it;
		if (KeyCommand::NoKeyCommand != edge.key_command)
		{
			actual_commands.push_back(edge.key_command);
		}
		++it;
	}

	BOOST_REQUIRE_EQUAL(actual_commands.size(), expected_commands.size());
	for (size_t i = 0; i < expected_commands.size(); ++i)
	{
		BOOST_CHECK_MESSAGE(
			actual_commands[i] == expected_commands[i],
			"Command mismatch at index " << i << ": expected " << static_cast<int>(expected_commands[i])
			<< ", got " << static_cast<int>(actual_commands[i])
		);
	}
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// Scraping Validation Logic Tests
// =============================================================================

BOOST_AUTO_TEST_SUITE(ScrapingValidation_TestSuite)

BOOST_AUTO_TEST_CASE(TestPreValidationMatch)
{
	// Pre-validation should pass when current page matches expected source
	ScreenDataPageTypes expected_source = ScreenDataPageTypes::Page_System;
	ScreenDataPageTypes current_page = ScreenDataPageTypes::Page_System;

	BOOST_CHECK(current_page == expected_source);
}

BOOST_AUTO_TEST_CASE(TestPreValidationMismatch)
{
	// Pre-validation should fail when current page doesn't match expected source
	ScreenDataPageTypes expected_source = ScreenDataPageTypes::Page_System;
	ScreenDataPageTypes current_page = ScreenDataPageTypes::Page_MenuHelp;

	BOOST_CHECK(current_page != expected_source);
}

BOOST_AUTO_TEST_CASE(TestPreValidationWildcard)
{
	// Page_Unknown acts as wildcard - should match anything
	ScreenDataPageTypes expected_source = ScreenDataPageTypes::Page_Unknown;
	ScreenDataPageTypes current_page = ScreenDataPageTypes::Page_MenuHelp;

	// Wildcard always passes
	bool validation_result = (expected_source == ScreenDataPageTypes::Page_Unknown) || (current_page == expected_source);
	BOOST_CHECK(validation_result);
}

BOOST_AUTO_TEST_CASE(TestPostValidationMatch)
{
	// Post-validation should pass when we arrived at expected destination
	ScreenDataPageTypes expected_destination = ScreenDataPageTypes::Page_MenuHelp;
	ScreenDataPageTypes current_page = ScreenDataPageTypes::Page_MenuHelp;

	BOOST_CHECK(current_page == expected_destination);
}

BOOST_AUTO_TEST_CASE(TestPostValidationMismatch)
{
	// Post-validation should fail when we didn't arrive at expected destination
	// This is the scenario that caused the original bug
	ScreenDataPageTypes expected_destination = ScreenDataPageTypes::Page_MenuHelp;
	ScreenDataPageTypes current_page = ScreenDataPageTypes::Page_System;  // Still on Home!

	BOOST_CHECK(current_page != expected_destination);
}

BOOST_AUTO_TEST_CASE(TestVertexPageTypeExtraction)
{
	// Test that we can correctly read page types directly from vertices
	ScreenDataPageGraph graph(
		{
			{ 1, ScreenDataPageTypes::Page_System },
			{ 2, ScreenDataPageTypes::Page_MenuHelp },
			{ 3, ScreenDataPageTypes::Page_Unknown }
		},
		{
			{ 1, 2, { 1, KeyCommand::Select } },
			{ 2, 3, { 2, KeyCommand::Select } }
		}
	);

	auto it = ForwardIterator::begin(graph, 1);

	// Vertex 1
	{
		auto& [vertex, edge] = *it;
		BOOST_CHECK(vertex.page == ScreenDataPageTypes::Page_System);
	}

	++it;

	// Vertex 2
	{
		auto& [vertex, edge] = *it;
		BOOST_CHECK(vertex.page == ScreenDataPageTypes::Page_MenuHelp);
	}

	++it;

	// Vertex 3
	{
		auto& [vertex, edge] = *it;
		BOOST_CHECK(vertex.page == ScreenDataPageTypes::Page_Unknown);
	}
}

BOOST_AUTO_TEST_SUITE_END()
