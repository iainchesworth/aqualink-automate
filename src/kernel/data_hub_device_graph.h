#pragma once

#include <memory>
#include <iterator>
#include <optional>

#include <boost/functional/hash.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/range/iterator_range.hpp>
#include <tl/expected.hpp>

#include "kernel/auxillary_base.h"
#include "logging/logging.h"

namespace AqualinkAutomate::Kernel
{

	using DeviceVertexProperty = std::shared_ptr<AuxillaryBase>;
	using DevicesGraphType = boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, DeviceVertexProperty>;
	using DeviceVertexType = boost::graph_traits<DevicesGraphType>::vertex_descriptor;
	using DeviceVertexIter = boost::graph_traits<DevicesGraphType>::vertex_iterator;
	using DeviceMap = std::unordered_map<uint32_t, std::shared_ptr<AuxillaryBase>>;

	namespace GraphManagement
	{
		template<typename TRAIT_TYPE>
		class DeviceTraitFilter
		{
		public:
			DeviceTraitFilter(const DevicesGraphType& graph, TRAIT_TYPE trait_type) :
				m_Graph(graph),
				m_TraitType(trait_type),
				m_OptTraitValue(std::nullopt)
			{
			}

			DeviceTraitFilter(const DevicesGraphType& graph, TRAIT_TYPE trait_type, TRAIT_TYPE::TraitValue trait_value) :
				m_Graph(graph),
				m_TraitType(trait_type),
				m_OptTraitValue(trait_value)
			{
			}

		public:
			bool operator()(const DevicesGraphType::edge_descriptor) const { return true; }
			bool operator()(const DevicesGraphType::vertex_descriptor vd) const
			{
				if (auto device = m_Graph[vd]; nullptr == device)
				{
					// Invalid device pointer
				}
				else if (!device->AuxillaryTraits.Has(m_TraitType))
				{
					// Trait does not exist
				}
				else
				{
					if (m_OptTraitValue.has_value() && (m_OptTraitValue.value() != device->AuxillaryTraits[m_TraitType]))
					{
						// Trait value didn't match
					}
					else
					{
						// Didn't need to validate trait value OR trait value matched.
						return true;
					}
				}

				return false;
			}

		private:
			const DevicesGraphType& m_Graph;
			TRAIT_TYPE m_TraitType;
			std::optional<typename TRAIT_TYPE::TraitValue> m_OptTraitValue;
		};
	}
	// namespace GraphManagement

	class DevicesGraph
	{
	public:
		DevicesGraph();

	public:
		void Add(std::shared_ptr<AuxillaryBase> device);
		void Remove(std::shared_ptr<AuxillaryBase> device);

	public:
		void CountByLabel();
		void FindByLabel();

	public:
		template<typename TRAIT_TYPE>
		uint32_t CountByTrait(TRAIT_TYPE trait_type) const
		{
			GraphManagement::DeviceTraitFilter<TRAIT_TYPE> trait_filter(m_DevicesGraph, trait_type);
			return CountByTraitImpl(trait_type, trait_filter);
		}
		
		template<typename TRAIT_TYPE>
		uint32_t CountByTrait(TRAIT_TYPE trait_type, TRAIT_TYPE::TraitValue trait_value) const 
		{
			GraphManagement::DeviceTraitFilter<TRAIT_TYPE> trait_filter(m_DevicesGraph, trait_type, trait_value);
			return CountByTraitImpl(trait_type, trait_filter);
		}

		template<typename DEVICE_TYPE, typename TRAIT_TYPE>
		std::vector<std::shared_ptr<DEVICE_TYPE>> FindByTrait(TRAIT_TYPE trait_type) const
		{
			GraphManagement::DeviceTraitFilter<TRAIT_TYPE> trait_filter(m_DevicesGraph, trait_type);
			return FindByTraitImpl<DEVICE_TYPE>(trait_type, trait_filter);
		}

		template<typename DEVICE_TYPE, typename TRAIT_TYPE>
		std::vector<std::shared_ptr<DEVICE_TYPE>> FindByTrait(TRAIT_TYPE trait_type, TRAIT_TYPE::TraitValue trait_value) const
		{
			GraphManagement::DeviceTraitFilter<TRAIT_TYPE> trait_filter(m_DevicesGraph, trait_type, trait_value);
			return FindByTraitImpl<DEVICE_TYPE>(trait_type, trait_filter);
		}

	private:
		template<typename TRAIT_TYPE>
		uint32_t CountByTraitImpl(TRAIT_TYPE trait_type, GraphManagement::DeviceTraitFilter<TRAIT_TYPE> trait_filter) const
		{
			boost::filtered_graph<DevicesGraphType, boost::keep_all, GraphManagement::DeviceTraitFilter<TRAIT_TYPE>> fg(m_DevicesGraph, boost::keep_all{}, trait_filter);

			auto range = boost::make_iterator_range(boost::vertices(fg));
			return std::distance(range.begin(), range.end());
		}

		template<typename DEVICE_TYPE, typename TRAIT_TYPE>
		std::vector<std::shared_ptr<DEVICE_TYPE>> FindByTraitImpl(TRAIT_TYPE trait_type, GraphManagement::DeviceTraitFilter<TRAIT_TYPE> trait_filter) const
		{
			boost::filtered_graph<DevicesGraphType, boost::keep_all, GraphManagement::DeviceTraitFilter<TRAIT_TYPE>> fg(m_DevicesGraph, boost::keep_all{}, trait_filter);

			std::vector<std::shared_ptr<DEVICE_TYPE>> found_devices;

			for (auto vp : boost::make_iterator_range(boost::vertices(fg)))
			{
				auto vertex_derived_ptr = std::dynamic_pointer_cast<DEVICE_TYPE>(m_DevicesGraph[vp]);

				if (nullptr != vertex_derived_ptr)
				{
					found_devices.push_back(std::move(vertex_derived_ptr));
				}
			}

			return found_devices;
		}

	private:
		template<typename DEVICE_TYPE>
		std::vector<std::shared_ptr<DEVICE_TYPE>> GetDevicesFromGraph(const DeviceVertexType& source_vertex) const
		{
			auto vertices_range = boost::adjacent_vertices(source_vertex, m_DevicesGraph);
			std::vector<std::shared_ptr<DEVICE_TYPE>> vertices;

			for (const auto& vertex : std::ranges::subrange(vertices_range.first, vertices_range.second))
			{
				if (auto baseptr = m_DevicesGraph[vertex]; nullptr == baseptr)
				{
					LogDebug(Channel::Equipment, "DataHub: Failed to retrieve baseptr from devices graph: baseptr was empty");
				}
				else if (auto aux_ptr = std::dynamic_pointer_cast<DEVICE_TYPE>(baseptr); nullptr == aux_ptr)
				{
					LogDebug(Channel::Equipment, "DataHub: Failed to convert baseptr to target type");
				}
				else
				{
					vertices.push_back(aux_ptr);
				}
			}

			return vertices;
		}

		template<typename DEVICE_TYPE>
		bool SearchGraphForExistingDevice(std::shared_ptr<DEVICE_TYPE> device, const DeviceVertexType& source_vertex) const
		{
			auto vertices_range = boost::adjacent_vertices(source_vertex, m_DevicesGraph);

			auto it = std::ranges::find_if(vertices_range.first, vertices_range.second, [&](const auto& possible_match_vertex) -> bool
				{
					if (nullptr == device)
					{
						LogDebug(Channel::Equipment, "DataHub: Failed to match device to vertex; expected valid device but it was empty");
					}
					else if (auto possible_match_baseptr = m_DevicesGraph[possible_match_vertex]; nullptr == possible_match_baseptr)
					{
						LogDebug(Channel::Equipment, "DataHub: Failed to match device to vertex; expected valid possible_match_baseptr but it was empty");
					}
					else if (auto ptr = std::dynamic_pointer_cast<DEVICE_TYPE>(m_DevicesGraph[possible_match_vertex]); nullptr == ptr)
					{
						LogDebug(Channel::Equipment, "DataHub: Failed to convert possible_match_baseptr to target type");
					}
					else
					{
						auto& lhs = *device;
						auto& rhs = *ptr;

						return (lhs == rhs);
					}

					return false;
				}
			);

			return (vertices_range.second != it);
		}
	
	private:
		DevicesGraphType m_DevicesGraph;
		DeviceVertexType m_RootVertexId;
		DeviceVertexType m_AuxilleriesVertexId;
		DeviceVertexType m_ChlorinatorsVertexId;
		DeviceVertexType m_HeatersVertexId;
		DeviceVertexType m_PumpsVertexId;
		DeviceMap m_DevicesMap;
	};	
	

}
// namespace AqualinkAutomate::Kernel
