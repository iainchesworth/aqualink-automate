#pragma once

#include <atomic>
#include <cstdint>
#include <format>
#include <string>
#include <unordered_map>
#include <variant>

#include <boost/signals2.hpp>
#include <magic_enum/magic_enum.hpp>

#include "exceptions/exception_signallingstatscounter_badaccess.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{

	using StatsSignalFunc = void(uint64_t);
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

	class SignallingStatsCounter
	{
	private:
		struct AnyEnum
		{
			template<typename STAT_TYPE>
				requires std::is_enum_v<STAT_TYPE>
			AnyEnum(STAT_TYPE value) :
				type_hash(typeid(STAT_TYPE).hash_code()),
				value_hash(std::hash<std::underlying_type_t<STAT_TYPE>>{}(static_cast<std::underlying_type_t<STAT_TYPE>>(value))),
				name(magic_enum::enum_name(value))
			{
			}

			std::size_t type_hash;
			std::size_t value_hash;
			std::string name;

			bool operator==(const AnyEnum& other) const noexcept
			{
				return type_hash == other.type_hash && value_hash == other.value_hash;
			}
		};

		struct AnyEnumHash
		{
			std::size_t operator()(const AnyEnum& key) const noexcept
			{
				return key.type_hash ^ (key.value_hash + 0x9e3779b9 + (key.type_hash << 6) + (key.type_hash >> 2));
			}
		};

	public:
		using StatsTypesMap = std::unordered_map<AnyEnum, StatsCounter, AnyEnumHash>;

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
		template<typename STAT_TYPE>
			requires std::is_enum_v<STAT_TYPE>
		StatsCounter& operator[](const STAT_TYPE& stat_type)
		{
			AnyEnum key{ stat_type };

			if (auto it = m_StatsMap.find(key); it != m_StatsMap.end())
			{
				return it->second;
			}

			AddStatsToCount(stat_type);

			return m_StatsMap.at(key);
		}

	public:
		template<typename STAT_TYPE>
			requires std::is_enum_v<STAT_TYPE>
		void AddStatsToCount(STAT_TYPE stat_type)
		{
			AnyEnum key{ stat_type };

			if (auto it = m_StatsMap.find(key); it == m_StatsMap.end())
			{
				if (auto [it_inserted, was_inserted] = m_StatsMap.emplace(key, StatsCounter(m_StatsSignal)); was_inserted)
				{
					LogTrace(Channel::Equipment, std::format("Inserted new stats counter for element (type: {})", typeid(STAT_TYPE).name()));
				}
				else
				{
					LogDebug(Channel::Equipment, std::format("Failed while inserting stats counter for element (type: {})", typeid(STAT_TYPE).name()));
					throw Exceptions::SignallingStatsCounter_BadAccess();
				}
			}
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
