#pragma once

#include <source_location>
#include <string>
#include <string_view>

#include <AMDTActivityLogger.h>

#include "profiling/profiling_units/frame.h"
#include "profiling/profiling_units/unit_colours.h"

namespace AqualinkAutomate::Profiling
{

	class UProfFrame : public Profiling::Frame
	{
	public:
		UProfFrame(std::string_view name, const std::source_location& src_loc = std::source_location::current(), UnitColours colour = UnitColours::NotSpecified);
		virtual ~UProfFrame() = default;

	public:
		virtual void Start() const override;
		virtual void Mark() const override;
		virtual void End() const override;

	private:
		std::string m_NameStr;
	};

}
// namespace AqualinkAutomate::Profiling
