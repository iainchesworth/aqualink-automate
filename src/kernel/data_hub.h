#pragma once

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

#include <boost/functional/hash.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/signals2.hpp>

#include "kernel/auxillary.h"
#include "kernel/circulation.h"
#include "kernel/data_hub_event.h"
#include "kernel/heater.h"
#include "kernel/orp.h"
#include "kernel/ph.h"
#include "kernel/pool_configurations.h"
#include "kernel/powercenter.h"
#include "kernel/pump.h"
#include "kernel/system_boards.h"
#include "jandy/equipment/jandy_equipment_modes.h"
#include "jandy/equipment/jandy_equipment_versions.h"
#include "jandy/utility/string_conversion/temperature.h"
#include "jandy/utility/string_conversion/timeout_duration.h"
#include "logging/logging.h"
#include "types/units_dimensionless.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Kernel
{

	class DataHub
	{
	public:
		DataHub();

	//---------------------------------------------------------------------
	// UPDATES / NOTIFICATIONS / EVENTS
	//---------------------------------------------------------------------

	public:
		mutable boost::signals2::signal<void(std::shared_ptr<Kernel::DataHub_Event>)> ConfigUpdateSignal;

	//---------------------------------------------------------------------
	// EQUIPMENT CONFIGURATION
	//---------------------------------------------------------------------

	public:
		Kernel::PoolConfigurations PoolConfiguration{ Kernel::PoolConfigurations::Unknown };
		Kernel::SystemBoards SystemBoard{ Kernel::SystemBoards::Unknown };

	//---------------------------------------------------------------------
	// EQUIPMENT STATUS
	//---------------------------------------------------------------------

	public:
		Equipment::JandyEquipmentModes Mode{ Equipment::JandyEquipmentModes::Normal };
		Utility::TimeoutDuration TimeoutRemaining;
		std::chrono::year_month_day Date{ std::chrono::year{2000}, std::chrono::month{1}, std::chrono::day{1} };
		std::chrono::hh_mm_ss<std::chrono::milliseconds> Time{ std::chrono::milliseconds(0) };

	//---------------------------------------------------------------------
	// EQUIPMENT VERSIONS
	//---------------------------------------------------------------------

	public:
		Equipment::JandyEquipmentVersions EquipmentVersions;

	//---------------------------------------------------------------------
	// CIRCULATION MODES
	//---------------------------------------------------------------------

	public:
		CirculationModes CirculationMode{ CirculationModes::Pool };

		bool SpaMode() const
		{
			return (CirculationModes::Spa == CirculationMode);
		}

		bool InCleanMode{ false };

	//---------------------------------------------------------------------
	// TEMPERATURES
	//---------------------------------------------------------------------

	public:
		Utility::Temperature AirTemp() const;
		Utility::Temperature PoolTemp() const;
		Utility::Temperature SpaTemp() const;
		Utility::Temperature FreezeProtectPoint() const;

	public:
		void AirTemp(const Utility::Temperature& air_temp);
		void PoolTemp(const Utility::Temperature& pool_temp);
		void SpaTemp(const Utility::Temperature& spa_temp);
		void FreezeProtectPoint(const Utility::Temperature& freeze_protect_point);

	private:
		Utility::Temperature m_AirTemp;
		Utility::Temperature m_PoolTemp;
		Utility::Temperature m_SpaTemp;
		Utility::Temperature m_FreezeProtectPoint;

	//---------------------------------------------------------------------
	// CHEMISTRY
	//---------------------------------------------------------------------
		 
	public:
		Kernel::ORP ORP() const;
		Kernel::pH pH() const;
		ppm_quantity SaltLevel() const;

	public:
		void ORP(const Kernel::ORP& orp);
		void pH(const Kernel::pH& pH);
		void SaltLevel(const ppm_quantity& salt_level_in_ppm);

	private:
		Kernel::ORP m_ORP{ 0.0f };
		Kernel::pH m_pH{ 0.0f };
		ppm_quantity m_SaltLevel{ 0 };

	//---------------------------------------------------------------------
	// DEVICES GRAPH
	//---------------------------------------------------------------------

	private:
		using DevicesGraphType = boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, std::shared_ptr<AuxillaryBase>>;
		using DeviceVertexType = boost::graph_traits<DevicesGraphType>::vertex_descriptor;
		using DeviceMap = std::unordered_map<uint32_t, std::shared_ptr<AuxillaryBase>>;

	public:
		void AddDevice(std::shared_ptr<AuxillaryBase> device);

	private:
		template<typename DEVICE_TYPE>
		bool SearchGraphForExistingDevice(std::shared_ptr<DEVICE_TYPE> device, const DeviceVertexType& source_vertex)
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

	public:
		std::vector<std::shared_ptr<Auxillary>> Auxillaries() const;
		std::vector<std::shared_ptr<Heater>> Heaters() const;
		std::vector<std::shared_ptr<Pump>> Pumps() const;

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

	private:
		DevicesGraphType m_DevicesGraph;
		DeviceVertexType m_AuxilleriesVertexId;
		DeviceVertexType m_HeatersVertexId;
		DeviceVertexType m_PumpsVertexId;
		DeviceMap m_DevicesMap;
	};

}
// namespace AqualinkAutomate::Equipment
