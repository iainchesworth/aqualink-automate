#pragma once

#include <chrono>
#include <memory>
#include <string>

#include <boost/assert/source_location.hpp>

#include "interfaces/iprofilingunit.h"
#include "profiling/profiling_units/unit_colours.h"

namespace AqualinkAutomate::Profiling
{

	class Zone : public Interfaces::IProfilingUnit
	{
	public:
		Zone(const std::string& name, const boost::source_location& src_loc = BOOST_CURRENT_LOCATION, UnitColours colour = UnitColours::NotSpecified);
		virtual ~Zone() = default;

	public:
		virtual void Start() const override;
		virtual void Mark() const override;
		virtual void End() const override;
	};

	using ZonePtr = std::unique_ptr<Profiling::Zone>;

}
// namespace AqualinkAutomate::Profiling
