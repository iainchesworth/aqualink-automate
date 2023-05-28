#pragma once

#include <any>
#include <limits>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_utility.hpp>

#include "jandy/utility/screen_data_page_graph.h"
#include "jandy/utility/screen_data_page_graph/screen_data_page_graph_edge.h"
#include "jandy/utility/screen_data_page_graph/screen_data_page_graph_vertex.h"

namespace AqualinkAutomate::Utility::ScreenDataPageGraphImpl
{

    class ForwardIterator
    {
    public:
        using value_type = std::tuple<VertexId, Edge>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type const *;
        using reference = value_type const &;
        using iterator_category = std::forward_iterator_tag;

    public:
        ForwardIterator(const ScreenDataPageGraph& graph, VertexId start_id) : 
            m_Graph(graph), 
            m_CurrentStep(start_id, Edge{}) 
        {
        }

        ForwardIterator& operator++()
        {
            auto vertex_id = std::get<VertexId>(m_CurrentStep);
            auto vertex_type = m_Graph.m_VertexMap.at(vertex_id);

            auto out_edge_range = boost::make_iterator_range(boost::out_edges(vertex_type, m_Graph.m_Graph));
            for (const auto& edge : out_edge_range)
            {
                auto target_vertex = boost::target(edge, m_Graph.m_Graph);
                auto target_id = m_Graph.m_Graph[target_vertex].id;

                if (m_Visited.find(target_id) == m_Visited.end())
                {
                    Edge edge_data = boost::get(boost::edge_bundle, m_Graph.m_Graph, edge);
                    m_CurrentStep = { target_id, edge_data };
                    m_Visited.insert(target_id);
                    return *this;
                }
            }

            // If no unvisited vertices are found, reset the state
            m_CurrentStep = { std::numeric_limits<VertexId>::max(), Edge{} };
            m_Visited.clear();
            return *this;
        }

        ForwardIterator operator++(int)
        {
            ForwardIterator temp = *this;
            ++(*this);
            return temp;
        }

        ForwardIterator::reference operator*() const
        {
            return m_CurrentStep;
        }

        ForwardIterator::pointer operator->() const
        {
            return &m_CurrentStep;
        }

        bool operator==(const ForwardIterator& other) const
        {
            return std::get<VertexId>(m_CurrentStep) == std::get<VertexId>(other.m_CurrentStep);
        }

        bool operator!=(const ForwardIterator& other) const
        {
            return !(*this == other);
        }

        static ForwardIterator begin(const ScreenDataPageGraph& graph, VertexId start_id)
        {
            return ForwardIterator(graph, start_id);
        }

        static ForwardIterator end(const ScreenDataPageGraph& graph)
        {
            return ForwardIterator(graph, std::numeric_limits<VertexId>::max());
        }

    private:
        const ScreenDataPageGraph& m_Graph;
        value_type m_CurrentStep;
        std::unordered_set<VertexId> m_Visited;
    };

}
// using namespace AqualinkAutomate::Utility::ScreenDataPageGraphImpl
