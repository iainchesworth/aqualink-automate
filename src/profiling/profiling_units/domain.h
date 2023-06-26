#pragma once

#include <memory>

#include <boost/assert/source_location.hpp>

#include "interfaces/iprofilingunit.h"
#include "profiling/profiling_units/unit_colours.h"

namespace AqualinkAutomate::Profiling
{

	class Domain : public Interfaces::IProfilingUnit
	{
	public:
		Domain(const std::string& name, const boost::source_location& src_loc = BOOST_CURRENT_LOCATION, UnitColours colour = UnitColours::NotSpecified);
		virtual ~Domain() = default;

	public:
		virtual void Start() const override;
		virtual void Mark() const override;
		virtual void End() const override;
	};

	using DomainPtr = std::unique_ptr<Profiling::Domain>;

}
// namespace AqualinkAutomate::Profiling
