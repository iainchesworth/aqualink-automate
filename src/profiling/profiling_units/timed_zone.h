#pragma once

#include <chrono>
#include <memory>
#include <source_location>
#include <string>

#include "interfaces/iprofilingunit.h"

namespace AqualinkAutomate::Profiling
{

	enum class TimedZoneStates
	{
		Paused,
		Running
	};

	class TimedZone : public Interfaces::IProfilingUnit
	{
	public:
		TimedZone(const std::string& name, TimedZoneStates start_state = TimedZoneStates::Running);
		virtual ~TimedZone() = default;

	public:
		virtual void Start() override;
		virtual void Mark() override;
		virtual void End() override;

	public:
		auto Duration() const;
		TimedZoneStates State() const;

	private:
		std::chrono::high_resolution_clock::time_point m_StartTime;
		std::chrono::high_resolution_clock::time_point m_EndTime;
		TimedZoneStates m_Started;
	};

}
// namespace AqualinkAutomate::Profiling
