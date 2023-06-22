#include "profiling/profiling_units/zone.h"

namespace AqualinkAutomate::Profiling
{

	Zone::Zone(const std::string& name, const boost::source_location& src_loc, UnitColours colour) :
		Interfaces::IProfilingUnit(name)
	{
	}

	inline void Zone::Start() const
	{
	}

	inline void Zone::Mark() const
	{
	}

	inline void Zone::End() const
	{
	}

}
// namespace AqualinkAutomate::Profiling
