#include <algorithm>

#include <tracy/Tracy.hpp>

#include "profiling/profiling_units/tracy_frame.h"

namespace AqualinkAutomate::Profiling
{

	TracyFrame::TracyFrame(const std::string& name, const std::source_location& src_loc, UnitColours colour) :
		Profiling::Frame(name, src_loc, colour),
		m_NamePtr(new char[Frame::Name().size() + 1])
	{
		if (nullptr != m_NamePtr)
		{
			std::fill(&m_NamePtr[0], &m_NamePtr[Frame::Name().size() + 1], 0);
			std::copy(Frame::Name().cbegin(), Frame::Name().cend(), m_NamePtr);
		}
	}

	TracyFrame::~TracyFrame()
	{
		if (nullptr != m_NamePtr)
		{
			delete[] m_NamePtr;
			m_NamePtr = nullptr;
		}
	}

	void TracyFrame::Start()
	{
		FrameMarkStart(m_NamePtr);
	}

	void TracyFrame::Mark()
	{
		FrameMark;
	}

	void TracyFrame::End()
	{
		FrameMarkEnd(m_NamePtr);
	}

}
// namespace AqualinkAutomate::Profiling
