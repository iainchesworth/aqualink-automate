#include "profiling/profiler_registry.h"
#include "profiling/profiler_types.h"
#include "profiling/uprof_profiler.h"

namespace AqualinkAutomate::Profiling
{

	static ProfilerAutoRegister<UProf_Profiler> reg_UProfProfiler(ProfilerTypes::UProf);

	void UProf_Profiler::StartProfiling()
	{
	}

	void UProf_Profiler::StopProfiling()
	{
	}

	void UProf_Profiler::MeasureZone(const Zone& zone)
	{
	}

}
// namespace AqualinkAutomate::Profiling
