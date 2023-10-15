#pragma once

#include <atomic>
#include <cstdint>
#include <format>
#include <string>
#include <unordered_map>
#include <variant>

#include <boost/signals2.hpp>

#include "exceptions/exception_signallingstatscounter_badaccess.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{

	using StatsSignalFunc = void();
	using StatsSignal = boost::signals2::signal<StatsSignalFunc>;

	//---------------------------------------------------------------------
	// 
	//---------------------------------------------------------------------

	class StatsCounter
	{
	public:
		StatsCounter(StatsSignal& stat_signal);
		StatsCounter(const StatsCounter& other);
		StatsCounter(StatsCounter&& other) noexcept;
		StatsCounter& operator=(const StatsCounter& other);
		StatsCounter& operator=(StatsCounter&& other) noexcept;

	public:
		StatsCounter& operator=(const uint64_t count_to_assign);
		StatsCounter& operator+=(const uint64_t count_to_add);
		StatsCounter& operator++();
		StatsCounter& operator++(int);

	public:
		uint64_t operator()() const;
		uint64_t Count() const;

	private:
		std::atomic_uint_fast64_t m_Count;
		StatsSignal& m_StatsSignal;
	};

	//---------------------------------------------------------------------
	// 
	//---------------------------------------------------------------------

	template<typename... STATS_TYPES>
	class SignallingStatsCounter
	{
	public:
		using StatsTypes = std::variant<STATS_TYPES...>;
		using StatsTypesMap = std::unordered_map<StatsTypes, StatsCounter>;

	public:
		using key_type = typename StatsTypesMap::key_type;
		using mapped_type = typename StatsTypesMap::mapped_type;
		using value_type = typename StatsTypesMap::value_type;

	public:
		SignallingStatsCounter() :
			m_StatsMap(),
			m_StatsSignal()
		{
		}

	public:
		StatsCounter& operator[](const StatsTypes& stat_type)
		{
			if (auto it = m_StatsMap.find(stat_type); it != m_StatsMap.end())
			{
				return it->second;
			}
			else
			{
				if (auto [it, was_inserted] = m_StatsMap.emplace(stat_type, StatsCounter(m_StatsSignal)); was_inserted)
				{
					LogTrace(Channel::Equipment, std::format("Inserted new stats counter for element: {}", std::hash<StatsTypes>{}(stat_type)));
					return it->second;
				}
				else
				{
					LogDebug(Channel::Equipment, std::format("Failed while inserting stats counter for element: {}", std::hash<StatsTypes>{}(stat_type)));
				}
			}

			throw Exceptions::SignallingStatsCounter_BadAccess();
		}

	public:
		auto begin() const noexcept { return m_StatsMap.begin(); }
		auto end() const noexcept { return m_StatsMap.end(); }
		auto cbegin() const noexcept { return m_StatsMap.cbegin(); }
		auto cend() const noexcept { return m_StatsMap.cend(); }

	public:
		StatsSignal& Signal() const
		{
			return m_StatsSignal;
		}

	private:
		StatsTypesMap m_StatsMap;
		mutable StatsSignal m_StatsSignal;
	};

}
// namespace AqualinkAutomate::Utility
