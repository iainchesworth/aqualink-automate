#pragma once

#include <source_location>
#include <string_view>

#include <ittnotify.h>

#include "profiling/profiling_units/unit_colours.h"
#include "profiling/profiling_units/zone.h"

namespace AqualinkAutomate::Profiling
{

	class VTuneZone : public Profiling::Zone
	{
	public:
		VTuneZone(std::string_view name, const std::source_location& src_loc = std::source_location::current(), UnitColours colour = UnitColours::NotSpecified);
		~VTuneZone() override;

	public:
		void Start() const override;
		void Mark() const override;
		void End() const override;

	public:
		void Text(std::string_view text) const override;
		void Value(uint64_t value) const override;

	private:
		__itt_domain* m_Domain;
		__itt_string_handle* m_Name;
	};

}
// namespace AqualinkAutomate::Profiling
