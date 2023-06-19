#include "profiling/profiling_units/tracy_zone_datamap.h"

namespace AqualinkAutomate::Profiling
{

	TracyZone_DataMap::DataMap& TracyZone_DataMap::Instance()
	{
		static DataMap zone_map;
		return zone_map;
	}

}
// namespace AqualinkAutomate::Profiling
