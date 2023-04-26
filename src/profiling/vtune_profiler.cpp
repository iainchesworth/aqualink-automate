#include "profiling/vtune_profiler.h"
#include "profiling/factories/profiler_factory_registration.h"
#include "profiling/types/profiling_types.h"

namespace AqualinkAutomate::Profiling
{

	const Factory::ProfilerRegistration<VTune_Profiler> VTune_Profiler::g_VTuneProfilerRegistration(AqualinkAutomate::Types::ProfilerTypes::VTune);
	const Factory::ProfilingUnitRegistration<Profiling::Domain, Profiling::Frame, Profiling::Zone> g_VTuneProfilingUnitRegistration(AqualinkAutomate::Types::ProfilerTypes::VTune);

	void VTune_Profiler::StartProfiling()
	{
	}

	void VTune_Profiler::StopProfiling()
	{
	}

}
// namespace AqualinkAutomate::Profiling
