#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
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
		void Remove(const std::shared_ptr<AuxillaryDevice>& device);

	public:
		uint32_t CountByLabel(std::string_view device_label) const;
		std::vector<std::shared_ptr<AuxillaryDevice>> FindByLabel(std::string_view device_label) const;

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
		template<typename Filter>
		auto MakeFilteredView(const Filter& filter) const
		{
			return boost::filtered_graph<DevicesGraphType, boost::keep_all, Filter>(m_DevicesGraph, boost::keep_all{}, filter);
		}

		template<typename TRAIT_TYPE>
		uint32_t CountByTraitImpl(TRAIT_TYPE trait_type, DeviceTraitFilter<TRAIT_TYPE> trait_filter) const
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("DeviceGraph::CountByTraitImpl", std::source_location::current());

			auto fg = MakeFilteredView(trait_filter);

			auto range = boost::make_iterator_range(boost::vertices(fg));

			return std::distance(range.begin(), range.end());
		}

		template<typename TRAIT_TYPE>
		std::vector<std::shared_ptr<AuxillaryDevice>> FindByTraitImpl(TRAIT_TYPE trait_type, DeviceTraitFilter<TRAIT_TYPE> trait_filter) const
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("DeviceGraph::FindByTraitImpl", std::source_location::current());

			std::vector<std::shared_ptr<AuxillaryDevice>> found_devices;

			auto fg = MakeFilteredView(trait_filter);

			for (auto vp : boost::make_iterator_range(boost::vertices(fg)))
			{
				if (auto device_ptr = m_DevicesGraph[vp]; nullptr != device_ptr)
				{
					found_devices.push_back(std::move(device_ptr));
				}
			}

			return found_devices;
		}
	
		// NOTE: The device graph is intentionally unsynchronised. Device
		// registration (from the protocol task) and queries (from the
		// HTTP/diagnostics handlers) both run on the single application thread
		// driven by the main poll() loop, so no locking is required. If a
		// multi-threaded execution model is ever reintroduced, this container
		// must be guarded before concurrent iteration/mutation.

	private:
		DevicesGraphType m_DevicesGraph;
		DeviceVertexType m_RootVertexId;
	};
	

}
// namespace AqualinkAutomate::Kernel
