#pragma once

#include <source_location>

#include <tracy/Tracy.hpp>

#include "profiling/profiling_units/unit_colours.h"
#include "profiling/profiling_units/zone.h"

namespace AqualinkAutomate::Profiling
{

	class TracyZone : public Profiling::Zone
	{
	public:
		TracyZone(const std::string name, const std::source_location& src_loc = std::source_location::current(), UnitColours colour = UnitColours::NotSpecified);
		virtual ~TracyZone();

	public:
		virtual void Start() const override;
		virtual void Mark() const override;
		virtual void End() const override;

	private:
		volatile tracy::ScopedZone* m_TSZ{ nullptr };
	};

}
// namespace AqualinkAutomate::Profiling
