#include <functional>
#include <unordered_map>
#include <unordered_set>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/test/unit_test.hpp>

#include "jandy/utility/screen_data_page_graph.h"
#include "logging/logging.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Utility;

// Define a fixture for the unit tests
/*
{
    using TestGraph = ScreenNavigator<int, int, char, bool>;
    using TestValidator = TestGraph::Validator;

    TestGraph graph;

    ScreenNavigatorGraphFixture()
    {
        graph.AddVertex(1, 6);
        graph.AddVertex(2, 7);
        graph.AddVertex(3, 8);
        graph.AddDefaultEdge(1, 9, 'A');
        graph.AddDefaultEdge(2, 10, 'B');
        graph.SetStartId(1);
        graph.SetEndId(3);
    }
};

BOOST_FIXTURE_TEST_SUITE(ScreenNavigatorGraphTests, ScreenNavigatorGraphFixture)

BOOST_AUTO_TEST_CASE(AddVertexTest)
{
    BOOST_CHECK_EQUAL(TestValidator(graph).Validate(), true);
    BOOST_CHECK_EQUAL(graph.GetVertexCount(), 3);

    BOOST_TEST(true == graph.AddVertex(4, 11));

    BOOST_CHECK_EQUAL(TestValidator(graph).Validate(), false); // Non-end vertex has no outgoing edges
    BOOST_CHECK_EQUAL(graph.GetVertexCount(), 4);
}

BOOST_AUTO_TEST_CASE(AddDefaultEdgeTest)
{
    BOOST_CHECK_EQUAL(TestValidator(graph).Validate(), true);
    BOOST_CHECK_EQUAL(graph.GetVertexCount(), 3);

    BOOST_TEST(false == graph.AddDefaultEdge(0, 1, 'C')); // No Source Vertex
    BOOST_TEST(false == graph.AddDefaultEdge(1, 5, 'C')); // No Destination Vertex
    BOOST_TEST(false == graph.AddDefaultEdge(1, 2, 'C')); // Default edge already exists

    BOOST_TEST(true == graph.AddVertex(4, 11));
    BOOST_CHECK_EQUAL(graph.GetVertexCount(), 4);
    BOOST_TEST(true == graph.AddDefaultEdge(3, 4, 'C'));

    BOOST_CHECK_EQUAL(TestValidator(graph).Validate(), false); // Non-end vertex has no outgoing edges

    graph.SetEndId(4);

    BOOST_CHECK_EQUAL(TestValidator(graph).Validate(), true);
}

BOOST_AUTO_TEST_CASE(AddEdgeTest)
{
    graph.AddEdge(1, 3, 'D', [](bool condition) { return condition; });
    BOOST_CHECK_EQUAL(TestValidator(graph).Validate(), true);
}

/*BOOST_AUTO_TEST_CASE(TraversalTest)
{
    std::string path;

    for (const auto& [_, edge_data] : graph)
    {
        path += std::to_string(edge_data) + " ";
    }

    BOOST_CHECK_EQUAL(path, "6 7 8 ");
}


BOOST_AUTO_TEST_SUITE_END()
*/