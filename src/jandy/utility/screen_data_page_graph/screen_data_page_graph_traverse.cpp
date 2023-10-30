#include <any>
#include <limits>

#include "jandy/utility/screen_data_page_graph/screen_data_page_graph_traverse.h"

namespace AqualinkAutomate::Utility::ScreenDataPageGraphImpl
{

    ForwardIterator::ForwardIterator(ScreenDataPageGraph& graph, VertexId start_id) :
        m_Graph(graph),
        m_CurrentStep({ start_id, std::any() }, Edge{})
    {
        if (m_Graph.m_VertexMap.end() != m_Graph.m_VertexMap.find(start_id))
        {
            // The provided start_id exists so map the vertex into the value_type.
            const auto& vertex_desc = m_Graph.m_VertexMap.at(start_id);
            std::get<Vertex>(m_CurrentStep) = m_Graph.m_Graph[vertex_desc];
        }
    }

    ForwardIterator::ForwardIterator(const ForwardIterator& other) :
        m_Graph(other.m_Graph),
        m_CurrentStep(other.m_CurrentStep),
        m_Visited(other.m_Visited)
    {
    }

    ForwardIterator::ForwardIterator(ForwardIterator&& other) noexcept :
        m_Graph(other.m_Graph),
        m_CurrentStep(std::move(other.m_CurrentStep)),
        m_Visited(std::move(other.m_Visited))
    {
    }

    ForwardIterator& ForwardIterator::operator=(const ForwardIterator& other)
    {
        if (this != &other)
        {
            m_Graph = other.m_Graph;
            m_CurrentStep = other.m_CurrentStep;
            m_Visited = other.m_Visited;
        }

        return *this;
    }

    ForwardIterator& ForwardIterator::operator=(ForwardIterator&& other) noexcept
    {
        if (this != &other)
        {
            m_Graph = std::move(other.m_Graph);
            m_CurrentStep = std::move(other.m_CurrentStep);
            m_Visited = std::move(other.m_Visited);
        }

        return *this;
    }

    ForwardIterator& ForwardIterator::operator++()
    {
        auto vertex_id = std::get<Vertex>(m_CurrentStep).id;
        auto vertex_type = m_Graph.m_VertexMap.at(vertex_id);

        auto out_edge_range = boost::make_iterator_range(boost::out_edges(vertex_type, m_Graph.m_Graph));
        for (const auto& edge : out_edge_range)
        {
            auto target_vertex = boost::target(edge, m_Graph.m_Graph);
            auto target_id = m_Graph.m_Graph[target_vertex].id;

            if (m_Visited.find(target_id) == m_Visited.end())
            {
                Edge edge_data = boost::get(boost::edge_bundle, m_Graph.m_Graph, edge);
                m_Visited.insert(target_id);

                const auto& vertex_desc = m_Graph.m_VertexMap.at(target_id);
                m_CurrentStep = { { target_id, m_Graph.m_Graph[vertex_desc] }, edge_data };
                return *this;
            }
        }

        // If no unvisited vertices are found, reset the state
        m_CurrentStep = { { std::numeric_limits<VertexId>::max(), std::any() }, Edge{} };
        m_Visited.clear();
        return *this;
    }

    ForwardIterator ForwardIterator::operator++(int)
    {
        ForwardIterator temp = *this;
        ++(*this);
        return temp;
    }

    ForwardIterator::reference ForwardIterator::operator*() const
    {
        return m_CurrentStep;
    }

    ForwardIterator::pointer ForwardIterator::operator->() const
    {
        return &m_CurrentStep;
    }

    bool ForwardIterator::operator==(const ForwardIterator& other) const
    {
        return std::get<Vertex>(m_CurrentStep).id == std::get<Vertex>(other.m_CurrentStep).id;
    }

    bool ForwardIterator::operator!=(const ForwardIterator& other) const
    {
        return !(*this == other);
    }

    ForwardIterator ForwardIterator::begin(ScreenDataPageGraph& graph, VertexId start_id)
    {
        return ForwardIterator(graph, start_id);
    }

    ForwardIterator ForwardIterator::end(ScreenDataPageGraph& graph)
    {
        return ForwardIterator(graph, std::numeric_limits<VertexId>::max());
    }

}
// using namespace AqualinkAutomate::Utility::ScreenDataPageGraphImpl
