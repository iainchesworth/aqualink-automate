#pragma once

#include <memory>

#include <boost/assert/source_location.hpp>

#include "interfaces/iprofilingunit.h"
#include "profiling/profiling_units/unit_colours.h"

namespace AqualinkAutomate::Profiling
{

	class Frame : public Interfaces::IProfilingUnit
	{
	public:
		Frame(const std::string& name, const boost::source_location& src_loc = BOOST_CURRENT_LOCATION, UnitColours colour = UnitColours::NotSpecified);
		virtual ~Frame() = default;

	public:
		virtual void Start() const override;
		virtual void Mark() const override;
		virtual void End() const override;
	};

	using FramePtr = std::unique_ptr<Profiling::Frame>;

}
// namespace AqualinkAutomate::Profiling
