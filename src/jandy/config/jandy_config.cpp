#include "jandy/config/jandy_config.h"

namespace AqualinkAutomate::Config
{
	
	JandyConfig::JandyConfig()
	{
		m_AuxilleriesVertexId = boost::add_vertex(std::shared_ptr<AuxillaryBase>(nullptr), m_DevicesGraph);
		m_HeatersVertexId = boost::add_vertex(std::shared_ptr<AuxillaryBase>(nullptr), m_DevicesGraph);
		m_PumpsVertexId = boost::add_vertex(std::shared_ptr<AuxillaryBase>(nullptr), m_DevicesGraph);
	}

	void JandyConfig::AddDevice(std::shared_ptr<AuxillaryBase> device)
	{
		auto insert_device_in_graph = [&](auto& m_DevicesGraph, auto& source_vertex, auto& ptr) -> void
		{
			auto target_vertex = boost::add_vertex(ptr, m_DevicesGraph);
			auto edge = boost::add_edge(source_vertex, target_vertex, m_DevicesGraph);

			if (auto [_, was_inserted] = m_DevicesMap.try_emplace(target_vertex, ptr); !was_inserted)
			{
				LogDebug(Channel::Equipment, "JandyConfig: Failed to add device to device unordered_map");
			}
		};

		if (nullptr == device)
		{
			LogDebug(Channel::Equipment, "JandyConfig: Failed to add device to device graph; invalid device provided for insertion");
		}
		else if (auto ptr = std::dynamic_pointer_cast<Auxillary>(device); (nullptr != ptr) && !SearchGraphForExistingDevice<Auxillary>(ptr, m_AuxilleriesVertexId))
		{
			insert_device_in_graph(m_DevicesGraph, m_AuxilleriesVertexId, ptr);
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
			LogDebug(Channel::Equipment, "JandyConfig: Failed to add device to device graph");
		}
	}

	std::vector<std::shared_ptr<Auxillary>> JandyConfig::Auxillaries() const
	{
		return GetDevicesFromGraph<Auxillary>(m_AuxilleriesVertexId);
	}

	std::vector<std::shared_ptr<Heater>> JandyConfig::Heaters() const
	{
		return GetDevicesFromGraph<Heater>(m_HeatersVertexId);
	}

	std::vector<std::shared_ptr<Pump>> JandyConfig::Pumps() const
	{
		return GetDevicesFromGraph<Pump>(m_PumpsVertexId);
	}

}
// namespace AqualinkAutomate::Config
