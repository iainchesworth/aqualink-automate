#include "profiling/profiling_units/domain.h"

namespace AqualinkAutomate::Profiling
{

	Domain::Domain(const std::string& name, const boost::source_location& src_loc, UnitColours colour) :
		Interfaces::IProfilingUnit(name)
	{
	}

	inline void Domain::Start() const
	{
	}

	inline void Domain::End() const
	{
	}

	inline void Domain::Mark() const
	{
	}

}
// namespace AqualinkAutomate::Profiling
