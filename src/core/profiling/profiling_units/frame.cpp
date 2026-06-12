#include "profiling/profiling_units/frame.h"

namespace AqualinkAutomate::Profiling
{

	Frame::Frame(std::string_view name, const std::source_location& src_loc, UnitColours colour) :
		Interfaces::IProfilingUnit(name)
	{
	}

	void Frame::Start() const
	{
	}

	void Frame::End() const
	{
	}

	void Frame::Mark() const
	{
	}

}
// namespace AqualinkAutomate::Profiling
