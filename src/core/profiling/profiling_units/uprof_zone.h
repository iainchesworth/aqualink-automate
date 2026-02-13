#pragma once

#include <source_location>
#include <string>
#include <string_view>

#include <AMDTActivityLogger.h>

#include "profiling/profiling_units/unit_colours.h"
#include "profiling/profiling_units/zone.h"

namespace AqualinkAutomate::Profiling
{

	class UProfZone : public Profiling::Zone
	{
	public:
		UProfZone(std::string_view name, const std::source_location& src_loc = std::source_location::current(), UnitColours colour = UnitColours::NotSpecified);
		virtual ~UProfZone();

	public:
		virtual void Start() const override;
		virtual void Mark() const override;
		virtual void End() const override;

	public:
		void Text(std::string_view text) const override;
		void Value(uint64_t value) const override;

	private:
		std::string m_NameStr;
	};

}
// namespace AqualinkAutomate::Profiling
