#pragma once

#include <memory>
#include <source_location>

#include "interfaces/iprofilingunit.h"
#include "profiling/profiling_units/unit_colours.h"

namespace AqualinkAutomate::Profiling
{

	class Frame : public Interfaces::IProfilingUnit
	{
	public:
		Frame(const std::string& name, const std::source_location& src_loc = std::source_location::current(), UnitColours colour = UnitColours::NotSpecified);
		virtual ~Frame() = default;

	public:
		virtual void Start() override;
		virtual void Mark() override;
		virtual void End() override;
	};

	using FramePtr = std::shared_ptr<Profiling::Frame>;

}
// namespace AqualinkAutomate::Profiling
