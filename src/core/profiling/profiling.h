#pragma once

#if defined (TRACY_ENABLE)

#include <tracy/Tracy.hpp>

#endif // defined (TRACY_ENABLE)

#include "profiling/factories/profiler_factory.h"
#include "profiling/factories/profiling_unit_factory.h"
#include "profiling/formatters/profiling_formatters.h"
#include "profiling/profiling_units/domain.h"
#include "profiling/profiling_units/frame.h"
#include "profiling/profiling_units/zone.h"
#include "profiling/profiling_units/unit_colours.h"
#include "profiling/types/profiling_types.h"

namespace AqualinkAutomate::Profiling
{

	void RegisterAvailableProfilers(Factory::ProfilerFactory& profiler, Factory::ProfilingUnitFactory& profiler_units);

}
// namespace AqualinkAutomate::Profiling
