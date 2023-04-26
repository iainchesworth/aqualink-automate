#include "profiling/profiling_units/frame.h"

namespace AqualinkAutomate::Profiling
{

	Frame::Frame(const std::string& name, const std::source_location& src_loc, UnitColours colour) :
		Interfaces::IProfilingUnit(name)
	{
	}

	void Frame::Start()
	{
	}

	void Frame::End()
	{
	}

	void Frame::Mark()
	{
	}

}
// namespace AqualinkAutomate::Profiling
