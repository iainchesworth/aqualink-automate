#pragma once

#include "interfaces/iprofiler.h"

namespace AqualinkAutomate::Profiling
{

	class NoOp_Profiler : public Interfaces::IProfiler
	{
	public:
		void StartProfiling() override;
		void StopProfiling() override;
	};

}
// namespace AqualinkAutomate::Profiling
