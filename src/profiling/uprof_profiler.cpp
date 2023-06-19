#include "profiling/uprof_profiler.h"
#include "profiling/factories/profiler_factory_registration.h"
#include "profiling/types/profiling_types.h"

namespace AqualinkAutomate::Profiling
{

	const Factory::ProfilerRegistration<UProf_Profiler> UProf_Profiler::g_UProfProfilerRegistration(AqualinkAutomate::Types::ProfilerTypes::UProf);
	const Factory::ProfilingUnitRegistration<Profiling::Domain, Profiling::Frame, Profiling::Zone> g_UProfProfilingUnitRegistration(AqualinkAutomate::Types::ProfilerTypes::UProf);

	void UProf_Profiler::StartProfiling()
	{
	}

	void UProf_Profiler::StopProfiling()
	{
	}

}
// namespace AqualinkAutomate::Profiling
