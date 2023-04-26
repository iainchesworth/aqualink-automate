#pragma once

#include "interfaces/iprofiler.h"

namespace AqualinkAutomate::Profiling
{

	class NoOp_Profiler : public Interfaces::IProfiler
	{
	public:
		virtual void StartProfiling() override;
		virtual void StopProfiling() override;
	};

}
// namespace AqualinkAutomate::Profiling
