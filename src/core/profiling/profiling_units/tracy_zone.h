#pragma once

#include <cstdint>
#include <memory>
#include <source_location>
#include <string_view>

#include <tracy/Tracy.hpp>

#include "profiling/profiling_units/unit_colours.h"
#include "profiling/profiling_units/zone.h"

namespace AqualinkAutomate::Profiling
{

	class TracyZone final : public Profiling::Zone
	{
	public:
		TracyZone(std::string_view name, const std::source_location& src_loc = std::source_location::current(), UnitColours colour = UnitColours::NotSpecified);
		~TracyZone() override = default;

	public:
		void Start() const override;
		void Mark() const override;
		void End() const override;

	public:
		void Text(std::string_view text) const override;
		void Value(uint64_t value) const override;

	private:
		std::unique_ptr<tracy::ScopedZone> m_TSZ;
	};

}
// namespace AqualinkAutomate::Profiling
