#pragma once

#include <memory>
#include <source_location>

#include "profiling/profiling_units/frame.h"
#include "profiling/profiling_units/unit_colours.h"

namespace AqualinkAutomate::Profiling
{

	class TracyFrame : public Profiling::Frame
	{
	public:
		TracyFrame(const std::string& name, const std::source_location& src_loc = std::source_location::current(), UnitColours colour = UnitColours::NotSpecified);
		virtual ~TracyFrame();

	public:
		virtual void Start() override;
		virtual void Mark() override;
		virtual void End() override;

	private:
		char* m_NamePtr{ nullptr };
	};

}
// namespace AqualinkAutomate::Profiling
