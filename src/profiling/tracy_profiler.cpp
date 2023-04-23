#include "profiling/profiler_registry.h"
#include "profiling/profiler_types.h"
#include "profiling/tracy_profiler.h"

namespace AqualinkAutomate::Profiling
{

	static ProfilerAutoRegister<Tracy_Profiler> reg_UProfProfiler(ProfilerTypes::Tracy);

	void Tracy_Profiler::StartProfiling()
	{
	}

	void Tracy_Profiler::StopProfiling()
	{
	}

	void Tracy_Profiler::MeasureZone(const Zone& zone)
	{
	}

}
// namespace AqualinkAutomate::Profiling
