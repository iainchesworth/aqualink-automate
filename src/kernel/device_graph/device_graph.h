#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

#include <boost/graph/filtered_graph.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/uuid/uuid.hpp>

#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/device_graph/device_graph_filter_by_trait.h"
#include "kernel/device_graph/device_graph_types.h"
#include "logging/logging.h"
#include "profiling/profiling.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Kernel
{

	class DevicesGraph
	{
	public:
		DevicesGraph();

	public:
		void Add(std::shared_ptr<AuxillaryDevice> device);
		bool Contains(std::shared_ptr<AuxillaryDevice> device) const;
		void Remove(std::shared_ptr<AuxillaryDevice> device);

	public:
		uint32_t CountByLabel(const std::string& device_label) const;
		std::vector<std::shared_ptr<AuxillaryDevice>> FindByLabel(const std::string& device_label) const;

	public:
		uint32_t CountById(const boost::uuids::uuid& id) const;
		std::shared_ptr<AuxillaryDevice> FindById(const boost::uuids::uuid& id) const;

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

		template<typename TRAIT_TYPE>
		std::vector<std::shared_ptr<AuxillaryDevice>> FindByTrait(TRAIT_TYPE trait_type) const
		{
			DeviceTraitFilter<TRAIT_TYPE> trait_filter(m_DevicesGraph, trait_type);
			return FindByTraitImpl(trait_type, trait_filter);
		}

		template<typename TRAIT_TYPE>
		std::vector<std::shared_ptr<AuxillaryDevice>> FindByTrait(TRAIT_TYPE trait_type, TRAIT_TYPE::TraitValue trait_value) const
		{
			DeviceTraitFilter<TRAIT_TYPE> trait_filter(m_DevicesGraph, trait_type, trait_value);
			return FindByTraitImpl(trait_type, trait_filter);
		}

	private:
		template<typename TRAIT_TYPE>
		uint32_t CountByTraitImpl(TRAIT_TYPE trait_type, DeviceTraitFilter<TRAIT_TYPE> trait_filter) const
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("DeviceGraph -> CountByTraitImpl", BOOST_CURRENT_LOCATION);

			std::shared_lock<std::shared_mutex> guard(m_GraphWriteLockMutex);

			boost::filtered_graph<DevicesGraphType, boost::keep_all, DeviceTraitFilter<TRAIT_TYPE>> fg(m_DevicesGraph, boost::keep_all{}, trait_filter);

			auto range = boost::make_iterator_range(boost::vertices(fg));

			return std::distance(range.begin(), range.end());
		}

		template<typename TRAIT_TYPE>
		std::vector<std::shared_ptr<AuxillaryDevice>> FindByTraitImpl(TRAIT_TYPE trait_type, DeviceTraitFilter<TRAIT_TYPE> trait_filter) const
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("DeviceGraph -> FindByTraitImpl", BOOST_CURRENT_LOCATION);

			std::vector<std::shared_ptr<AuxillaryDevice>> found_devices;

			std::shared_lock<std::shared_mutex> guard(m_GraphWriteLockMutex);

			boost::filtered_graph<DevicesGraphType, boost::keep_all, DeviceTraitFilter<TRAIT_TYPE>> fg(m_DevicesGraph, boost::keep_all{}, trait_filter);

			for (auto vp : boost::make_iterator_range(boost::vertices(fg)))
			{
				if (auto device_ptr = m_DevicesGraph[vp]; nullptr != device_ptr)
				{
					found_devices.push_back(std::move(device_ptr));
				}
			}

			return std::move(found_devices);
		}
	
	private:
		DevicesGraphType m_DevicesGraph;
		DeviceVertexType m_RootVertexId;
		DeviceMap m_DevicesMap;

	private:
		mutable std::shared_mutex m_GraphWriteLockMutex{};
	};	
	

}
// namespace AqualinkAutomate::Kernel
