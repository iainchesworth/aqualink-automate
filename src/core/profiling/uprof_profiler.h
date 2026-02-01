#pragma once

#include "interfaces/iprofiler.h"
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
	};	

}
// namespace AqualinkAutomate::Profiling
