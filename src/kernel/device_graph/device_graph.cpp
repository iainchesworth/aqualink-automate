#include "kernel/auxillary_devices/auxillary.h"
#include "kernel/auxillary_devices/chlorinator.h"
#include "kernel/auxillary_devices/heater.h"
#include "kernel/auxillary_devices/pump.h"
#include "kernel/device_graph/device_graph.h"
#include "kernel/device_graph/device_graph_filter_by_id.h"
#include "kernel/device_graph/device_graph_filter_by_label.h"

namespace AqualinkAutomate::Kernel
{
	
	DevicesGraph::DevicesGraph()
	{
		m_RootVertexId = boost::add_vertex(std::shared_ptr<AuxillaryBase>(nullptr), m_DevicesGraph);
		m_AuxilleriesVertexId = boost::add_vertex(std::shared_ptr<AuxillaryBase>(nullptr), m_DevicesGraph);
		m_ChlorinatorsVertexId = boost::add_vertex(std::shared_ptr<AuxillaryBase>(nullptr), m_DevicesGraph);
		m_HeatersVertexId = boost::add_vertex(std::shared_ptr<AuxillaryBase>(nullptr), m_DevicesGraph);
		m_PumpsVertexId = boost::add_vertex(std::shared_ptr<AuxillaryBase>(nullptr), m_DevicesGraph);

		boost::add_edge(m_RootVertexId, m_AuxilleriesVertexId, m_DevicesGraph);
		boost::add_edge(m_RootVertexId, m_ChlorinatorsVertexId, m_DevicesGraph);
		boost::add_edge(m_RootVertexId, m_HeatersVertexId, m_DevicesGraph);
		boost::add_edge(m_RootVertexId, m_PumpsVertexId, m_DevicesGraph);
	}

	void DevicesGraph::Add(std::shared_ptr<AuxillaryBase> device)
	{
		auto insert_device_in_graph = [&](auto& m_DevicesGraph, auto& source_vertex, auto& ptr) -> void
		{
			auto target_vertex = boost::add_vertex(ptr, m_DevicesGraph);
			auto edge = boost::add_edge(source_vertex, target_vertex, m_DevicesGraph);

			if (auto [_, was_inserted] = m_DevicesMap.try_emplace(target_vertex, ptr); !was_inserted)
			{
				LogDebug(Channel::Equipment, "DataHub: Failed to add device to device unordered_map");
			}
		};

		using AuxillaryTraitsTypes::AuxillaryTypeTrait;
		using AuxillaryTraitsTypes::AuxillaryTypes;

		if (nullptr == device)
		{
			LogDebug(Channel::Equipment, "DataHub: Failed to add device to device graph -> invalid device provided for insertion");
		}
		else if (Contains(device))
		{
			LogTrace(Channel::Equipment, "DataHub: Did not add device to device graph -> device already exists");
		}
		else if (!(device->AuxillaryTraits.Has(AuxillaryTypeTrait{})))
		{
			insert_device_in_graph(m_DevicesGraph, m_RootVertexId, device);
		}
		else
		{
			switch (*(device->AuxillaryTraits[AuxillaryTypeTrait{}]))
			{
			case AuxillaryTypes::Auxillary:
				insert_device_in_graph(m_DevicesGraph, m_AuxilleriesVertexId, device);
				break;

			case AuxillaryTypes::Chlorinator:
				insert_device_in_graph(m_DevicesGraph, m_ChlorinatorsVertexId, device);
				break;

			case AuxillaryTypes::Heater:
				insert_device_in_graph(m_DevicesGraph, m_HeatersVertexId, device);
				break;

			case AuxillaryTypes::Pump:
				insert_device_in_graph(m_DevicesGraph, m_PumpsVertexId, device);
				break;

			default:
				LogDebug(Channel::Equipment, "DataHub: Failed to add device to device graph -> unknown AuxillaryTypeTrait");
				break;
			}
		}
	}

	bool DevicesGraph::Contains(std::shared_ptr<AuxillaryBase> device)
	{
		if (nullptr == device)
		{
			return false;
		}
		else
		{
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

	void DevicesGraph::Remove(std::shared_ptr<AuxillaryBase> device)
	{
	}

	uint32_t DevicesGraph::CountById(const boost::uuids::uuid& id) const
	{
		DeviceIdFilter filter(m_DevicesGraph, id);

		boost::filtered_graph<DevicesGraphType, boost::keep_all, DeviceIdFilter> fg(m_DevicesGraph, boost::keep_all{}, filter);

		auto range = boost::make_iterator_range(boost::vertices(fg));
		return std::distance(range.begin(), range.end());
	}

	std::shared_ptr<AuxillaryBase> DevicesGraph::FindById(const boost::uuids::uuid& id) const
	{
		DeviceIdFilter filter(m_DevicesGraph, id);

		boost::filtered_graph<DevicesGraphType, boost::keep_all, DeviceIdFilter> fg(m_DevicesGraph, boost::keep_all{}, filter);

		for (auto vp : boost::make_iterator_range(boost::vertices(fg)))
		{
			// Use the first device with a matching id.
			return m_DevicesGraph[vp];
		}

		return nullptr;
	}

	uint32_t DevicesGraph::CountByLabel(const std::string& device_label)
	{
		DeviceLabelFilter filter(m_DevicesGraph, device_label);

		boost::filtered_graph<DevicesGraphType, boost::keep_all, DeviceLabelFilter> fg(m_DevicesGraph, boost::keep_all{}, filter);

		auto range = boost::make_iterator_range(boost::vertices(fg));
		return std::distance(range.begin(), range.end());
	}

	std::vector<std::shared_ptr<AuxillaryBase>> DevicesGraph::FindByLabel(const std::string& device_label)
	{
		DeviceLabelFilter filter(m_DevicesGraph, device_label);

		boost::filtered_graph<DevicesGraphType, boost::keep_all, DeviceLabelFilter> fg(m_DevicesGraph, boost::keep_all{}, filter);

		std::vector<std::shared_ptr<AuxillaryBase>> found_devices;

		for (auto vp : boost::make_iterator_range(boost::vertices(fg)))
		{
			found_devices.push_back(m_DevicesGraph[vp]);
		}

		return found_devices;
	}

}
// namespace AqualinkAutomate::Kernel
