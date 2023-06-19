#pragma once

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

#include <boost/functional/hash.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

#include "jandy/config/jandy_config_auxillary.h"
#include "jandy/config/jandy_config_circulation.h"
#include "jandy/config/jandy_config_heater.h"
#include "jandy/config/jandy_config_pool_configurations.h"
#include "jandy/config/jandy_config_powercenter.h"
#include "jandy/config/jandy_config_pump.h"
#include "jandy/config/jandy_config_system_boards.h"
#include "jandy/equipment/jandy_equipment_modes.h"
#include "jandy/equipment/jandy_equipment_versions.h"
#include "jandy/utility/string_conversion/temperature.h"
#include "jandy/utility/string_conversion/timeout_duration.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Config
{

	class JandyConfig
	{
		using DevicesGraphType = boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, std::shared_ptr<AuxillaryBase>>;
		using DeviceVertexType = boost::graph_traits<DevicesGraphType>::vertex_descriptor;
		using DeviceMap = std::unordered_map<uint32_t, std::shared_ptr<AuxillaryBase>>;

	public:
		JandyConfig();

	public:
		Config::PoolConfigurations PoolConfiguration{ Config::PoolConfigurations::Unknown };
		Config::SystemBoards SystemBoard{ Config::SystemBoards::Unknown };

	public:
		Equipment::JandyEquipmentModes Mode{ Equipment::JandyEquipmentModes::Normal };
		Utility::TimeoutDuration TimeoutRemaining;
		std::chrono::year_month_day Date{ std::chrono::year{2000}, std::chrono::month{1}, std::chrono::day{1} };
		std::chrono::hh_mm_ss<std::chrono::milliseconds> Time{ std::chrono::milliseconds(0) };

	public:
		CirculationModes CirculationMode{ CirculationModes::Pool };

		bool SpaMode() const
		{
			return (CirculationModes::Spa == CirculationMode);
		}

		bool InCleanMode{ false };

	public:
		Utility::Temperature AirTemp;
		Utility::Temperature PoolTemp;
		Utility::Temperature SpaTemp;
		Utility::Temperature FreezeProtectPoint;

	public:
		Equipment::JandyEquipmentVersions EquipmentVersions;

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
						LogDebug(Channel::Equipment, "JandyConfig: Failed to match device to vertex; expected valid device but it was empty");
					}
					else if (auto possible_match_baseptr = m_DevicesGraph[possible_match_vertex]; nullptr == possible_match_baseptr)
					{
						LogDebug(Channel::Equipment, "JandyConfig: Failed to match device to vertex; expected valid possible_match_baseptr but it was empty");
					}
					else if (auto ptr = std::dynamic_pointer_cast<DEVICE_TYPE>(m_DevicesGraph[possible_match_vertex]); nullptr == ptr)
					{
						LogDebug(Channel::Equipment, "JandyConfig: Failed to convert possible_match_baseptr to target type");
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
					LogDebug(Channel::Equipment, "JandyConfig: Failed to retrieve baseptr from devices graph: baseptr was empty");
				}
				else if (auto aux_ptr = std::dynamic_pointer_cast<DEVICE_TYPE>(baseptr); nullptr == aux_ptr)
				{
					LogDebug(Channel::Equipment, "JandyConfig: Failed to convert baseptr to target type");
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
