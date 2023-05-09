#pragma once

#include <memory>
#include <source_location>

#include "interfaces/iprofilingunit.h"
#include "profiling/profiling_units/unit_colours.h"

namespace AqualinkAutomate::Profiling
{

	class Domain : public Interfaces::IProfilingUnit
	{
	public:
		Domain(const std::string& name, const std::source_location& src_loc = std::source_location::current(), UnitColours colour = UnitColours::NotSpecified);
		virtual ~Domain() = default;

	public:
		virtual void Start() override;
		virtual void Mark() override;
		virtual void End() override;
	};

	using DomainPtr = std::shared_ptr<Profiling::Domain>;

}
// namespace AqualinkAutomate::Profiling