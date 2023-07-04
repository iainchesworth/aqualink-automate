#include "kernel/data_hub.h"

namespace AqualinkAutomate::Kernel
{
	
	DataHub::DataHub()
	{
		m_AuxilleriesVertexId = boost::add_vertex(std::shared_ptr<AuxillaryBase>(nullptr), m_DevicesGraph);
		m_HeatersVertexId = boost::add_vertex(std::shared_ptr<AuxillaryBase>(nullptr), m_DevicesGraph);
		m_PumpsVertexId = boost::add_vertex(std::shared_ptr<AuxillaryBase>(nullptr), m_DevicesGraph);
	}

	Utility::Temperature DataHub::AirTemp() const
	{
		return m_AirTemp;
	}

	Utility::Temperature DataHub::PoolTemp() const
	{
		return m_PoolTemp;
	}

	Utility::Temperature DataHub::SpaTemp() const
	{
		return m_SpaTemp;
	}

	Utility::Temperature DataHub::FreezeProtectPoint() const
	{
		return m_FreezeProtectPoint;
	}

	void DataHub::AirTemp(const Utility::Temperature& air_temp)
	{
		m_AirTemp = air_temp;

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_Event_Temperature>();
		update_event->AirTemp(m_AirTemp);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::PoolTemp(const Utility::Temperature& pool_temp)
	{
		m_PoolTemp = pool_temp;

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_Event_Temperature>();
		update_event->PoolTemp(m_PoolTemp);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::SpaTemp(const Utility::Temperature& spa_temp)
	{
		m_SpaTemp = spa_temp;

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_Event_Temperature>();
		update_event->SpaTemp(m_SpaTemp);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::FreezeProtectPoint(const Utility::Temperature& freeze_protect_point)
	{
		m_FreezeProtectPoint = freeze_protect_point;
	}

	void DataHub::AddDevice(std::shared_ptr<AuxillaryBase> device)
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

	std::vector<std::shared_ptr<Auxillary>> DataHub::Auxillaries() const
	{
		return GetDevicesFromGraph<Auxillary>(m_AuxilleriesVertexId);
	}

	std::vector<std::shared_ptr<Heater>> DataHub::Heaters() const
	{
		return GetDevicesFromGraph<Heater>(m_HeatersVertexId);
	}

	std::vector<std::shared_ptr<Pump>> DataHub::Pumps() const
	{
		return GetDevicesFromGraph<Pump>(m_PumpsVertexId);
	}

}
// namespace AqualinkAutomate::Kernel
