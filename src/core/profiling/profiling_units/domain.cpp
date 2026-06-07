#include "profiling/profiling_units/domain.h"

namespace AqualinkAutomate::Profiling
{

	Domain::Domain(std::string_view name, const std::source_location& src_loc, UnitColours colour) :
		Interfaces::IProfilingUnit(name)
	{
	}

	void Domain::Start() const
	{
	}

	void Domain::End() const
	{
	}

	void Domain::Mark() const
	{
	}

}
// namespace AqualinkAutomate::Profiling
