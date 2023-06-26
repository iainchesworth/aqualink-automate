#pragma once

#include <memory>
#include <tuple>

#include "profiling/factories/profiling_unit_factory.h"
#include "profiling/types/profiling_types.h"

namespace AqualinkAutomate::Factory
{
	template<typename DOMAIN_TYPE, typename FRAME_TYPE, typename ZONE_TYPE>
	class ProfilingUnitRegistration
	{
	public:
		ProfilingUnitRegistration(const Types::ProfilerTypes type)
		{
			Factory::ProfilingUnitFactory::Instance().Register(type, std::make_tuple(
				[](const std::string& name, const boost::source_location& src_loc, Profiling::UnitColours colour) -> Types::ProfilingUnitTypePtr { return std::make_unique<DOMAIN_TYPE>(name, src_loc, colour); },
				[](const std::string& name, const boost::source_location& src_loc, Profiling::UnitColours colour) -> Types::ProfilingUnitTypePtr { return std::make_unique<FRAME_TYPE>(name, src_loc, colour); },
				[](const std::string& name, const boost::source_location& src_loc, Profiling::UnitColours colour) -> Types::ProfilingUnitTypePtr { return std::make_unique<ZONE_TYPE>(name, src_loc, colour); }
			));
		}
	};
}
// namespace AqualinkAutomate::Factory
