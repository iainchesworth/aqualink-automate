#include "profiling/profiling_units/vtune_frame.h"

namespace AqualinkAutomate::Profiling
{

	static __itt_domain* GetVTuneDomain()
	{
		static auto* domain = __itt_domain_create("AqualinkAutomate");
		return domain;
	}

	VTuneFrame::VTuneFrame(std::string_view name, const std::source_location& src_loc, UnitColours colour) :
		Frame(name, src_loc, colour),
		m_Domain(GetVTuneDomain())
	{
	}

	void VTuneFrame::Start() const
	{
		__itt_frame_begin_v3(m_Domain, nullptr);
	}

	void VTuneFrame::Mark() const
	{
		// Frame boundary: end the previous frame (if open) and begin the next, so
		// the interval between marks is one frame instead of a zero-width pair.
		if (m_FrameOpen)
		{
			__itt_frame_end_v3(m_Domain, nullptr);
		}
		__itt_frame_begin_v3(m_Domain, nullptr);
		m_FrameOpen = true;
	}

	void VTuneFrame::End() const
	{
		__itt_frame_end_v3(m_Domain, nullptr);
		m_FrameOpen = false;
	}

}
// namespace AqualinkAutomate::Profiling
