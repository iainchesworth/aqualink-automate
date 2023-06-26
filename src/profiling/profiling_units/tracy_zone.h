#pragma once

#include <boost/assert/source_location.hpp>
#include <tracy/Tracy.hpp>

#include "profiling/profiling_units/unit_colours.h"
#include "profiling/profiling_units/zone.h"

namespace AqualinkAutomate::Profiling
{

	class TracyZone : public Profiling::Zone
	{
	public:
		TracyZone(const std::string name, const boost::source_location& src_loc = BOOST_CURRENT_LOCATION, UnitColours colour = UnitColours::NotSpecified);
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
