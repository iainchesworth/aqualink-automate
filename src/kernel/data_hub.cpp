#include <algorithm>
#include <execution>

#include "kernel/data_hub.h"
#include "kernel/data_hub_event_chemistry.h"
#include "kernel/data_hub_event_temperature.h"
#include "utility/case_insensitive_comparision.h"

namespace AqualinkAutomate::Kernel
{
	
	DataHub::DataHub()
	{
		m_AuxilleriesVertexId = boost::add_vertex(std::shared_ptr<AuxillaryBase>(nullptr), m_DevicesGraph);
		m_ChlorinatorsVertexId = boost::add_vertex(std::shared_ptr<AuxillaryBase>(nullptr), m_DevicesGraph);
		m_HeatersVertexId = boost::add_vertex(std::shared_ptr<AuxillaryBase>(nullptr), m_DevicesGraph);
		m_PumpsVertexId = boost::add_vertex(std::shared_ptr<AuxillaryBase>(nullptr), m_DevicesGraph);
	}

	Kernel::Temperature DataHub::AirTemp() const
	{
		return m_AirTemp;
	}

	Kernel::Temperature DataHub::PoolTemp() const
	{
		return m_PoolTemp;
	}

	Kernel::Temperature DataHub::SpaTemp() const
	{
		return m_SpaTemp;
	}

	Kernel::Temperature DataHub::FreezeProtectPoint() const
	{
		return m_FreezeProtectPoint;
	}

	void DataHub::AirTemp(const Kernel::Temperature& air_temp)
	{
		m_AirTemp = air_temp;

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_Event_Temperature>();
		update_event->AirTemp(m_AirTemp);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::PoolTemp(const Kernel::Temperature& pool_temp)
	{
		m_PoolTemp = pool_temp;

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_Event_Temperature>();
		update_event->PoolTemp(m_PoolTemp);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::SpaTemp(const Kernel::Temperature& spa_temp)
	{
		m_SpaTemp = spa_temp;

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_Event_Temperature>();
		update_event->SpaTemp(m_SpaTemp);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::FreezeProtectPoint(const Kernel::Temperature& freeze_protect_point)
	{
		m_FreezeProtectPoint = freeze_protect_point;
	}

	Kernel::ORP DataHub::ORP() const
	{
		return m_ORP;
	}

	Kernel::pH DataHub::pH() const
	{
		return m_pH;
	}

	ppm_quantity DataHub::SaltLevel() const
	{
		return m_SaltLevel;
	}

	void DataHub::ORP(const Kernel::ORP& orp)
	{
		m_ORP = orp;

		// Signal that a chemistry update has occurred.
		auto update_event = std::make_shared<DataHub_Event_Chemistry>();
		update_event->ORP(m_ORP);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::pH(const Kernel::pH& pH)
	{
		m_pH = pH;

		// Signal that a chemistry update has occurred.
		auto update_event = std::make_shared<DataHub_Event_Chemistry>();
		update_event->pH(m_pH);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::SaltLevel(const ppm_quantity& salt_level_in_ppm)
	{
		m_SaltLevel = salt_level_in_ppm;

		// Signal that a chemistry update has occurred.
		auto update_event = std::make_shared<DataHub_Event_Chemistry>();
		update_event->SaltLevel(m_SaltLevel);
		ConfigUpdateSignal(update_event);
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

	std::vector<std::shared_ptr<Chlorinator>> DataHub::Chlorinators() const
	{
		return GetDevicesFromGraph<Chlorinator>(m_ChlorinatorsVertexId);
	}

	std::vector<std::shared_ptr<Heater>> DataHub::Heaters() const
	{
		return GetDevicesFromGraph<Heater>(m_HeatersVertexId);
	}

	std::vector<std::shared_ptr<Pump>> DataHub::Pumps() const
	{
		return GetDevicesFromGraph<Pump>(m_PumpsVertexId);
	}

	std::optional<std::shared_ptr<Pump>> DataHub::FilterPump() const
	{
		static std::shared_ptr<Pump> filter_pump(nullptr);

		auto match_filter_pump = [](const auto& pump_ptr) -> bool
		{
			if (nullptr == pump_ptr)
			{
				// The pump object doesn't exist...ignore this device.
				return false;
			}
			else
			{
				const auto& pump_label = pump_ptr->Label();
				const std::string filter_pump{ "filter pump" };

				return std::equal(pump_label.cbegin(), pump_label.cend(), filter_pump.cbegin(), Utility::case_insensitive_comparision);
			}
		};

		if (nullptr != filter_pump)
		{
			// Already know which device is the filter pump so use the cache.
			return filter_pump;
		}
		else
		{
			const auto pumps_collection = Pumps();

			if (auto pump_it = std::find_if(std::execution::par, pumps_collection.cbegin(), pumps_collection.cend(), match_filter_pump); pumps_collection.end() == pump_it)
			{
				// Did not find the filter pump, do nothing.
			}
			else
			{
				// Found the filter pump, cache it and return it.
				filter_pump = (*pump_it);
				return filter_pump;
			}
		}

		return std::nullopt;
	}

}
// namespace AqualinkAutomate::Kernel
