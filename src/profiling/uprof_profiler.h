#pragma once

#include "interfaces/iprofiler.h"
#include "profiling/factories/profiler_factory_registration.h"
#include "profiling/factories/profiling_unit_factory_registration.h"
#include "profiling/profiling_units/domain.h"
#include "profiling/profiling_units/frame.h"
#include "profiling/profiling_units/zone.h"

namespace AqualinkAutomate::Profiling
{

	class UProf_Profiler : public Interfaces::IProfiler
	{
	public:
		virtual void StartProfiling() override;
		virtual void StopProfiling() override;

	private:
		static const Factory::ProfilerRegistration<UProf_Profiler> g_UProfProfilerRegistration;
		static const Factory::ProfilingUnitRegistration<Profiling::Domain, Profiling::Frame, Profiling::Zone> g_UProfProfilingUnitRegistration;
	};	

}
// namespace AqualinkAutomate::Profiling
