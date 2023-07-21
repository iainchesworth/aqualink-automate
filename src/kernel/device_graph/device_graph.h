#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <boost/graph/filtered_graph.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/uuid/uuid.hpp>

#include "kernel/auxillary_base.h"
#include "kernel/device_graph/device_graph_filter_by_trait.h"
#include "kernel/device_graph/device_graph_types.h"
#include "logging/logging.h"

namespace AqualinkAutomate::Kernel
{

	class DevicesGraph
	{
	public:
		DevicesGraph();

	public:
		void Add(std::shared_ptr<AuxillaryBase> device);
		
		
		bool Contains(std::shared_ptr<AuxillaryBase> device);


		void Remove(std::shared_ptr<AuxillaryBase> device);

	public:
		uint32_t CountByLabel(const std::string& device_label);
		std::vector<std::shared_ptr<AuxillaryBase>> FindByLabel(const std::string& device_label);

	public:
		uint32_t CountById(const boost::uuids::uuid& id) const;
		std::shared_ptr<AuxillaryBase> FindById(const boost::uuids::uuid& id) const;

	public:
		template<typename TRAIT_TYPE>
		uint32_t CountByTrait(TRAIT_TYPE trait_type) const
		{
			DeviceTraitFilter<TRAIT_TYPE> trait_filter(m_DevicesGraph, trait_type);
			return CountByTraitImpl(trait_type, trait_filter);
		}
		
		template<typename TRAIT_TYPE>
		uint32_t CountByTrait(TRAIT_TYPE trait_type, TRAIT_TYPE::TraitValue trait_value) const 
		{
			DeviceTraitFilter<TRAIT_TYPE> trait_filter(m_DevicesGraph, trait_type, trait_value);
			return CountByTraitImpl(trait_type, trait_filter);
		}

		template<typename DEVICE_TYPE, typename TRAIT_TYPE>
		std::vector<std::shared_ptr<DEVICE_TYPE>> FindByTrait(TRAIT_TYPE trait_type) const
		{
			DeviceTraitFilter<TRAIT_TYPE> trait_filter(m_DevicesGraph, trait_type);
			return FindByTraitImpl<DEVICE_TYPE>(trait_type, trait_filter);
		}

		template<typename DEVICE_TYPE, typename TRAIT_TYPE>
		std::vector<std::shared_ptr<DEVICE_TYPE>> FindByTrait(TRAIT_TYPE trait_type, TRAIT_TYPE::TraitValue trait_value) const
		{
			DeviceTraitFilter<TRAIT_TYPE> trait_filter(m_DevicesGraph, trait_type, trait_value);
			return FindByTraitImpl<DEVICE_TYPE>(trait_type, trait_filter);
		}

	private:
		template<typename TRAIT_TYPE>
		uint32_t CountByTraitImpl(TRAIT_TYPE trait_type, DeviceTraitFilter<TRAIT_TYPE> trait_filter) const
		{
			boost::filtered_graph<DevicesGraphType, boost::keep_all, DeviceTraitFilter<TRAIT_TYPE>> fg(m_DevicesGraph, boost::keep_all{}, trait_filter);

			auto range = boost::make_iterator_range(boost::vertices(fg));
			return std::distance(range.begin(), range.end());
		}

		template<typename DEVICE_TYPE, typename TRAIT_TYPE>
		std::vector<std::shared_ptr<DEVICE_TYPE>> FindByTraitImpl(TRAIT_TYPE trait_type, DeviceTraitFilter<TRAIT_TYPE> trait_filter) const
		{
			boost::filtered_graph<DevicesGraphType, boost::keep_all, DeviceTraitFilter<TRAIT_TYPE>> fg(m_DevicesGraph, boost::keep_all{}, trait_filter);

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
