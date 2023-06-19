#include "profiling/profiling_units/timed_zone.h"

namespace AqualinkAutomate::Profiling
{

	TimedZone::TimedZone(const std::string& name, TimedZoneStates start_state) :
		Interfaces::IProfilingUnit(name),
		m_StartTime(std::chrono::high_resolution_clock::now()),
		m_EndTime(m_StartTime),
		m_Started(start_state)
	{
		if (TimedZoneStates::Paused == m_Started)
		{
			m_StartTime = std::chrono::high_resolution_clock::time_point::min();
		}
	}

	inline void TimedZone::Start() const
	{
		if (TimedZoneStates::Paused == m_Started)
		{
			m_StartTime = std::chrono::high_resolution_clock::now();
			m_Started = TimedZoneStates::Running;
		}
	}

	inline void TimedZone::Mark() const
	{
	}

	inline void TimedZone::End() const
	{
		if (TimedZoneStates::Running == m_Started)
		{
			m_EndTime = std::chrono::high_resolution_clock::now();
		}
	}

	auto TimedZone::Duration() const
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(m_EndTime - m_StartTime);
	}

	TimedZoneStates TimedZone::State() const
	{
		return m_Started;
	}

}
// namespace AqualinkAutomate::Profiling
