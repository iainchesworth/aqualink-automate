#pragma once

#include <chrono>
#include <memory>
#include <source_location>
#include <string>

#include "interfaces/iprofilingunit.h"
#include "profiling/profiling_units/unit_colours.h"

namespace AqualinkAutomate::Profiling
{

	class Zone : public Interfaces::IProfilingUnit
	{
	public:
		Zone(std::string_view name, const std::source_location& src_loc = std::source_location::current(), UnitColours colour = UnitColours::NotSpecified);
		~Zone() override = default;

	public:
		void Start() const override;
		void Mark() const override;
		void End() const override;
	};

	using ZonePtr = std::unique_ptr<Profiling::Zone>;

}
// namespace AqualinkAutomate::Profiling
