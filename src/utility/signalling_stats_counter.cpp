#include "utility/signalling_stats_counter.h"

namespace AqualinkAutomate::Utility
{

	StatsCounter::StatsCounter(StatsSignal& stat_signal) :
		m_Count{ 0 },
		m_StatsSignal(stat_signal)
	{
	}

	StatsCounter::StatsCounter(const StatsCounter& other) :
		m_Count{ 0 },
		m_StatsSignal(other.m_StatsSignal)
	{
		m_Count.exchange(other.m_Count);
	}

	StatsCounter::StatsCounter(StatsCounter&& other) noexcept :
		m_Count{ 0 },
		m_StatsSignal(other.m_StatsSignal)
	{
		m_Count.exchange(other.m_Count);
	}

	StatsCounter& StatsCounter::operator=(const StatsCounter& other)
	{
		if (this != &other)
		{
			m_Count.exchange(other.m_Count);
			m_StatsSignal.connect(other.m_StatsSignal);
		}

		return *this;
	}

	StatsCounter& StatsCounter::operator=(StatsCounter&& other) noexcept
	{
		if (this != &other)
		{
			m_Count.exchange(other.m_Count);
			m_StatsSignal = std::move(other.m_StatsSignal);
		}

		return *this;
	}

	StatsCounter& StatsCounter::operator=(const uint64_t count_to_assign)
	{
		m_Count = count_to_assign;
		m_StatsSignal();
		return *this;
	}

	StatsCounter& StatsCounter::operator+=(const uint64_t count_to_add)
	{
		m_Count += count_to_add;
		m_StatsSignal(); 
		return *this;
	}

	StatsCounter& StatsCounter::operator++()
	{
		m_Count++;
		m_StatsSignal(); 
		return *this;
	}

	StatsCounter& StatsCounter::operator++(int)
	{
		++m_Count;
		m_StatsSignal(); 
		return *this;
	}

	uint64_t StatsCounter::operator()() const
	{
		return Count();
	}

	uint64_t StatsCounter::Count() const
	{
		return m_Count;
	}

}
// namespace AqualinkAutomate::Utility
