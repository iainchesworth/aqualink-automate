#pragma once

#include <memory>
#include <source_location>
#include <string_view>

#include "profiling/profiling_units/frame.h"
#include "profiling/profiling_units/unit_colours.h"

namespace AqualinkAutomate::Profiling
{

	class TracyFrame : public Profiling::Frame
	{
	public:
		TracyFrame(std::string_view name, const std::source_location& src_loc = std::source_location::current(), UnitColours colour = UnitColours::NotSpecified);
		~TracyFrame() override = default;

	public:
		void Start() const override;
		void Mark() const override;
		void End() const override;

	private:
		static const char* GetOrCacheFrameName(std::string_view name);

	private:
		const char* m_NamePtr{ nullptr };
	};

}
// namespace AqualinkAutomate::Profiling
