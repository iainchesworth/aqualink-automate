#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>

#include "profiling/tracy_profiler.h"
#include "profiling/factories/profiler_factory_registration.h"
#include "profiling/profiling_units/tracy_zone.h"
#include "profiling/types/profiling_types.h"

namespace AqualinkAutomate::Profiling
{

	const Factory::ProfilerRegistration<Tracy_Profiler>Tracy_Profiler::g_TracyProfilerRegistration(AqualinkAutomate::Types::ProfilerTypes::Tracy);
	const Factory::ProfilingUnitRegistration<Profiling::Domain, Profiling::TracyFrame, Profiling::TracyZone> g_TracyProfilingUnitRegistration(AqualinkAutomate::Types::ProfilerTypes::Tracy);

	void Tracy_Profiler::StartProfiling()
	{
	}

	void Tracy_Profiler::StopProfiling()
	{
		tracy::GetProfiler().RequestShutdown();
	}

	ZonePtr Tracy_Profiler::CreateZone(FramePtr frame, const std::string& name) const
	{
		return std::make_unique<TracyZone>(name);
	}

}
// namespace AqualinkAutomate::Profiling
