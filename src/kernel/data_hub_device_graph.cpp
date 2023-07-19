#include "kernel/auxillary.h"
#include "kernel/chlorinator.h"
#include "kernel/data_hub_device_graph.h"
#include "kernel/heater.h"
#include "kernel/pump.h"

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

		if (nullptr == device)
		{
			LogDebug(Channel::Equipment, "DataHub: Failed to add device to device graph; invalid device provided for insertion");
		}
		else if (auto ptr = std::dynamic_pointer_cast<Auxillary>(device); (nullptr != ptr) && !SearchGraphForExistingDevice<Auxillary>(ptr, m_AuxilleriesVertexId))
		{
			insert_device_in_graph(m_DevicesGraph, m_AuxilleriesVertexId, ptr);
		}
		else if (auto ptr = std::dynamic_pointer_cast<Chlorinator>(device); (nullptr != ptr) && !SearchGraphForExistingDevice<Chlorinator>(ptr, m_ChlorinatorsVertexId))
		{
			insert_device_in_graph(m_DevicesGraph, m_ChlorinatorsVertexId, ptr);
		}
		else if (auto ptr = std::dynamic_pointer_cast<Heater>(device); (nullptr != ptr) && !SearchGraphForExistingDevice<Heater>(ptr, m_HeatersVertexId))
		{
			insert_device_in_graph(m_DevicesGraph, m_HeatersVertexId, ptr);
		}
		else if (auto ptr = std::dynamic_pointer_cast<Pump>(device); (nullptr != ptr) && !SearchGraphForExistingDevice<Pump>(ptr, m_PumpsVertexId))
		{
			insert_device_in_graph(m_DevicesGraph, m_PumpsVertexId, ptr);
		}
		else
		{
			LogDebug(Channel::Equipment, "DataHub: Failed to add device to device graph");
		}
	}

	void DevicesGraph::Remove(std::shared_ptr<AuxillaryBase> device)
	{
	}

	void DevicesGraph::CountByLabel()
	{
	}

	void DevicesGraph::FindByLabel()
	{
	}

}
// namespace AqualinkAutomate::Kernel
