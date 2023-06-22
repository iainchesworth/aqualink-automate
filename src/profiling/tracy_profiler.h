#pragma once

#include "interfaces/iprofiler.h"
#include "profiling/factories/profiler_factory_registration.h"
#include "profiling/factories/profiling_unit_factory_registration.h"
#include "profiling/profiling_units/domain.h"
#include "profiling/profiling_units/tracy_frame.h"
#include "profiling/profiling_units/tracy_zone.h"

namespace AqualinkAutomate::Profiling
{

	class Tracy_Profiler : public Interfaces::IProfiler
	{
	public:
		virtual void StartProfiling() override;
		virtual void StopProfiling() override;

	public:
		virtual std::expected<ZonePtr, bool> CreateZone(FramePtr frame, const std::string& name) const override;

	private:
		static const Factory::ProfilerRegistration<Tracy_Profiler> g_TracyProfilerRegistration;
		static const Factory::ProfilingUnitRegistration<Profiling::Domain, Profiling::TracyFrame, Profiling::TracyZone> g_TracyProfilingUnitRegistration;
	};

}
// namespace AqualinkAutomate::Profiling
