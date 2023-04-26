#include "profiling/profiling_units/zone.h"

namespace AqualinkAutomate::Profiling
{

	Zone::Zone(const std::string& name, const std::source_location& src_loc, UnitColours colour) :
		Interfaces::IProfilingUnit(name)
	{
	}

	void Zone::Start()
	{
	}

	void Zone::Mark()
	{
	}

	void Zone::End()
	{
	}

}
// namespace AqualinkAutomate::Profiling
