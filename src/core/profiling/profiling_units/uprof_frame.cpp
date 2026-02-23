#include "profiling/profiling_units/uprof_frame.h"

namespace AqualinkAutomate::Profiling
{

	UProfFrame::UProfFrame(std::string_view name, const std::source_location& src_loc, UnitColours colour) :
		Frame(name, src_loc, colour),
		m_NameStr(name)
	{
	}

	void UProfFrame::Start() const
	{
		amdtBeginMarker(m_NameStr.c_str(), "Frames");
	}

	void UProfFrame::Mark() const
	{
		amdtBeginMarker(m_NameStr.c_str(), "Frames");
		amdtEndMarker();
	}

	void UProfFrame::End() const
	{
		amdtEndMarker();
	}

}
// namespace AqualinkAutomate::Profiling
