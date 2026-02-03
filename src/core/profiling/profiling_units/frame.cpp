#include "profiling/profiling_units/frame.h"

namespace AqualinkAutomate::Profiling
{

	Frame::Frame(std::string_view name, const std::source_location& src_loc, UnitColours colour) :
		Interfaces::IProfilingUnit(name)
	{
	}

	inline void Frame::Start() const
	{
	}

	inline void Frame::End() const
	{
	}

	inline void Frame::Mark() const
	{
	}

}
// namespace AqualinkAutomate::Profiling
