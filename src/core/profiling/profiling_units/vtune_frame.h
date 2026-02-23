#pragma once

#include <source_location>
#include <string_view>

#include <ittnotify.h>

#include "profiling/profiling_units/frame.h"
#include "profiling/profiling_units/unit_colours.h"

namespace AqualinkAutomate::Profiling
{

	class VTuneFrame : public Profiling::Frame
	{
	public:
		VTuneFrame(std::string_view name, const std::source_location& src_loc = std::source_location::current(), UnitColours colour = UnitColours::NotSpecified);
		~VTuneFrame() override = default;

	public:
		void Start() const override;
		void Mark() const override;
		void End() const override;

	private:
		__itt_domain* m_Domain;
	};

}
// namespace AqualinkAutomate::Profiling
