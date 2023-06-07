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
		Zone(const std::string& name, const std::source_location& src_loc = std::source_location::current(), UnitColours colour = UnitColours::NotSpecified);
		virtual ~Zone() = default;

	public:
		virtual void Start() const override;
		virtual void Mark() const override;
		virtual void End() const override;
	};

	using ZonePtr = std::unique_ptr<Profiling::Zone>;

}
// namespace AqualinkAutomate::Profiling
