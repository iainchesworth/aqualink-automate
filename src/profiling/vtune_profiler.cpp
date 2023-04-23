#include "profiling/profiler_registry.h"
#include "profiling/profiler_types.h"
#include "profiling/vtune_profiler.h"

namespace AqualinkAutomate::Profiling
{

	static ProfilerAutoRegister<VTune_Profiler> reg_VTuneProfiler(ProfilerTypes::VTune);

	void VTune_Profiler::StartProfiling()
	{
	}

	void VTune_Profiler::StopProfiling()
	{
	}

	void VTune_Profiler::MeasureZone(const Zone& zone)
	{
	}

}
// namespace AqualinkAutomate::Profiling
