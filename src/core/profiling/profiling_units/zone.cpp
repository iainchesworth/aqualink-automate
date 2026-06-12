#include "profiling/profiling_units/zone.h"

namespace AqualinkAutomate::Profiling
{

	Zone::Zone(std::string_view name, const std::source_location& src_loc, UnitColours colour) :
		Interfaces::IProfilingUnit(name)
	{
	}

	void Zone::Start() const
	{
	}

	void Zone::Mark() const
	{
	}

	void Zone::End() const
	{
	}

}
// namespace AqualinkAutomate::Profiling
