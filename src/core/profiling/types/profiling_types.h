#pragma once

#include <memory>

#include "interfaces/iprofiler.h"
#include "interfaces/iprofilingunit.h"

namespace AqualinkAutomate::Types
{

	using ProfilerTypePtr = std::shared_ptr<Interfaces::IProfiler>;
	using ProfilingUnitTypePtr = std::unique_ptr<Interfaces::IProfilingUnit>;

	enum class ProfilerTypes
	{
		Tracy,
		UProf,
		VTune
	};

}
// namespace AqualinkAutomate::Types
