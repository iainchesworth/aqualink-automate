#include "profiling/profiling.h"

#if defined(TRACY_ENABLE)
#include "profiling/tracy_profiler.h"
#include "profiling/profiling_units/tracy_frame.h"
#include "profiling/profiling_units/tracy_zone.h"
#endif

#if defined(UProf_SUPPORT_ENABLED)
#include "profiling/uprof_profiler.h"
#endif

#if defined(VTUNE_SUPPORT_ENABLED)
#include "profiling/vtune_profiler.h"
#endif

namespace AqualinkAutomate::Profiling
{

	void RegisterAvailableProfilers(Factory::ProfilerFactory& profiler, Factory::ProfilingUnitFactory& profiler_units)
	{
#if defined(TRACY_ENABLE)
		profiler.Register(Types::ProfilerTypes::Tracy, std::make_shared<Tracy_Profiler>());

		profiler_units.Register(Types::ProfilerTypes::Tracy, std::make_tuple(
			[](const std::string& name, const std::source_location& src_loc, UnitColours colour) -> Types::ProfilingUnitTypePtr
			{
				return std::make_unique<Domain>(name, src_loc, colour);
			},
			[](const std::string& name, const std::source_location& src_loc, UnitColours colour) -> Types::ProfilingUnitTypePtr
			{
				return std::make_unique<TracyFrame>(name, src_loc, colour);
			},
			[](const std::string& name, const std::source_location& src_loc, UnitColours colour) -> Types::ProfilingUnitTypePtr
			{
				return std::make_unique<TracyZone>(name, src_loc, colour);
			}
		));
#endif

#if defined(UProf_SUPPORT_ENABLED)
		profiler.Register(Types::ProfilerTypes::UProf, std::make_shared<UProf_Profiler>());

		profiler_units.Register(Types::ProfilerTypes::UProf, std::make_tuple(
			[](const std::string& name, const std::source_location& src_loc, UnitColours colour) -> Types::ProfilingUnitTypePtr
			{
				return std::make_unique<Domain>(name, src_loc, colour);
			},
			[](const std::string& name, const std::source_location& src_loc, UnitColours colour) -> Types::ProfilingUnitTypePtr
			{
				return std::make_unique<Frame>(name, src_loc, colour);
			},
			[](const std::string& name, const std::source_location& src_loc, UnitColours colour) -> Types::ProfilingUnitTypePtr
			{
				return std::make_unique<Zone>(name, src_loc, colour);
			}
		));
#endif

#if defined(VTUNE_SUPPORT_ENABLED)
		profiler.Register(Types::ProfilerTypes::VTune, std::make_shared<VTune_Profiler>());

		profiler_units.Register(Types::ProfilerTypes::VTune, std::make_tuple(
			[](const std::string& name, const std::source_location& src_loc, UnitColours colour) -> Types::ProfilingUnitTypePtr
			{
				return std::make_unique<Domain>(name, src_loc, colour);
			},
			[](const std::string& name, const std::source_location& src_loc, UnitColours colour) -> Types::ProfilingUnitTypePtr
			{
				return std::make_unique<Frame>(name, src_loc, colour);
			},
			[](const std::string& name, const std::source_location& src_loc, UnitColours colour) -> Types::ProfilingUnitTypePtr
			{
				return std::make_unique<Zone>(name, src_loc, colour);
			}
		));
#endif
	}

}
// namespace AqualinkAutomate::Profiling