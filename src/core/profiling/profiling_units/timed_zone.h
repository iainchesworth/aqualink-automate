#pragma once

#include <chrono>
#include <memory>
#include <string>

#include <boost/assert/source_location.hpp>

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
		virtual void Start() const override;
		virtual void Mark() const override;
		virtual void End() const override;

	public:
		auto Duration() const;
		TimedZoneStates State() const;

	private:
		mutable std::chrono::high_resolution_clock::time_point m_StartTime;
		mutable std::chrono::high_resolution_clock::time_point m_EndTime;
		mutable TimedZoneStates m_Started;
	};

}
// namespace AqualinkAutomate::Profiling
