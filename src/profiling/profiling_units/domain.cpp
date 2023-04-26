#include "profiling/profiling_units/domain.h"

namespace AqualinkAutomate::Profiling
{

	Domain::Domain(const std::string& name, const std::source_location& src_loc, UnitColours colour) :
		Interfaces::IProfilingUnit(name)
	{
	}

	void Domain::Start()
	{
	}

	void Domain::End()
	{
	}

	void Domain::Mark()
	{
	}

}
// namespace AqualinkAutomate::Profiling
