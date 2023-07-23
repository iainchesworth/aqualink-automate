#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/device_graph/device_graph.h"
#include "kernel/device_graph/device_graph_filter_by_id.h"
#include "kernel/device_graph/device_graph_filter_by_label.h"

namespace AqualinkAutomate::Kernel
{
	
	DevicesGraph::DevicesGraph()
	{
		m_RootVertexId = boost::add_vertex(std::shared_ptr<AuxillaryDevice>(nullptr), m_DevicesGraph);
	}

	void DevicesGraph::Add(std::shared_ptr<AuxillaryDevice> device)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("DeviceGraph -> Add", BOOST_CURRENT_LOCATION);

		auto insert_device_in_graph = [&](auto& m_DevicesGraph, auto& source_vertex, auto& ptr) -> void
		{
			std::unique_lock<std::shared_mutex> guard(m_GraphWriteLockMutex);

			auto target_vertex = boost::add_vertex(ptr, m_DevicesGraph);
			auto edge = boost::add_edge(source_vertex, target_vertex, m_DevicesGraph);

			if (auto [_, was_inserted] = m_DevicesMap.try_emplace(target_vertex, ptr); !was_inserted)
			{
				LogDebug(Channel::Equipment, "DataHub: Failed to add device to device unordered_map");
			}
		};

		if (nullptr == device)
		{
			LogDebug(Channel::Equipment, "DataHub: Failed to add device to device graph -> invalid device provided for insertion");
		}
		else if (Contains(device))
		{
			LogTrace(Channel::Equipment, "DataHub: Did not add device to device graph -> device already exists");
		}
		else
		{
			insert_device_in_graph(m_DevicesGraph, m_RootVertexId, device);
		}
	}

	bool DevicesGraph::Contains(std::shared_ptr<AuxillaryDevice> device) const
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("DeviceGraph -> Contains", BOOST_CURRENT_LOCATION);

		if (nullptr == device)
		{
			return false;
		}
		else
		{
			std::shared_lock<std::shared_mutex> guard(m_GraphWriteLockMutex);

			auto iter = std::find_if
			(
				boost::vertices(m_DevicesGraph).first,
				boost::vertices(m_DevicesGraph).second,
				[this, &device](const auto& vertex)
				{
					if (nullptr == m_DevicesGraph[vertex])
					{
						return false;
					}
					else
					{
						// Compare the *objects* not the pointers...
						return (*(m_DevicesGraph[vertex]) == *device);
					}
				}
			);

			return (boost::vertices(m_DevicesGraph).second != iter);
		}
	}

	void DevicesGraph::Remove(std::shared_ptr<AuxillaryDevice> device)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("DeviceGraph -> Remove", BOOST_CURRENT_LOCATION);

		std::unique_lock<std::shared_mutex> guard(m_GraphWriteLockMutex);
	}

	uint32_t DevicesGraph::CountById(const boost::uuids::uuid& id) const
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("DeviceGraph -> CountById", BOOST_CURRENT_LOCATION);

		DeviceIdFilter filter(m_DevicesGraph, id);

		std::shared_lock<std::shared_mutex> guard(m_GraphWriteLockMutex);

		boost::filtered_graph<DevicesGraphType, boost::keep_all, DeviceIdFilter> fg(m_DevicesGraph, boost::keep_all{}, filter);

		auto range = boost::make_iterator_range(boost::vertices(fg));

		return std::distance(range.begin(), range.end());
	}

	std::shared_ptr<AuxillaryDevice> DevicesGraph::FindById(const boost::uuids::uuid& id) const
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("DeviceGraph -> FindById", BOOST_CURRENT_LOCATION);

		DeviceIdFilter filter(m_DevicesGraph, id);

		std::shared_lock<std::shared_mutex> guard(m_GraphWriteLockMutex);

		boost::filtered_graph<DevicesGraphType, boost::keep_all, DeviceIdFilter> fg(m_DevicesGraph, boost::keep_all{}, filter);

		for (auto vp : boost::make_iterator_range(boost::vertices(fg)))
		{
			// Use the first device with a matching id.
			return m_DevicesGraph[vp];
		}

		return nullptr;
	}

	uint32_t DevicesGraph::CountByLabel(const std::string& device_label) const
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("DeviceGraph -> CountByLabel", BOOST_CURRENT_LOCATION);

		DeviceLabelFilter filter(m_DevicesGraph, device_label);

		std::shared_lock<std::shared_mutex> guard(m_GraphWriteLockMutex);

		boost::filtered_graph<DevicesGraphType, boost::keep_all, DeviceLabelFilter> fg(m_DevicesGraph, boost::keep_all{}, filter);

		auto range = boost::make_iterator_range(boost::vertices(fg));
		return std::distance(range.begin(), range.end());
	}

	std::vector<std::shared_ptr<AuxillaryDevice>> DevicesGraph::FindByLabel(const std::string& device_label) const
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("DeviceGraph -> FindByLabel", BOOST_CURRENT_LOCATION);

		DeviceLabelFilter filter(m_DevicesGraph, device_label);

		std::shared_lock<std::shared_mutex> guard(m_GraphWriteLockMutex);

		boost::filtered_graph<DevicesGraphType, boost::keep_all, DeviceLabelFilter> fg(m_DevicesGraph, boost::keep_all{}, filter);

		std::vector<std::shared_ptr<AuxillaryDevice>> found_devices;

		for (auto vp : boost::make_iterator_range(boost::vertices(fg)))
		{
			found_devices.push_back(m_DevicesGraph[vp]);
		}

		return found_devices;
	}

}
// namespace AqualinkAutomate::Kernel
