#pragma once

#include <memory>

#include <boost/assert/source_location.hpp>

#include "profiling/profiling_units/frame.h"
#include "profiling/profiling_units/unit_colours.h"

namespace AqualinkAutomate::Profiling
{

	class TracyFrame : public Profiling::Frame
	{
	public:
		TracyFrame(const std::string& name, const boost::source_location& src_loc = BOOST_CURRENT_LOCATION, UnitColours colour = UnitColours::NotSpecified);
		virtual ~TracyFrame();

	public:
		virtual void Start() const override;
		virtual void Mark() const override;
		virtual void End() const override;

	private:
		char* m_NamePtr{ nullptr };
	};

}
// namespace AqualinkAutomate::Profiling
