#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "interfaces/iprofiler.h"
#include "profiling/profiling_units/domain.h"
#include "profiling/profiling_units/tracy_frame.h"
#include "profiling/profiling_units/tracy_zone.h"

namespace AqualinkAutomate::Profiling
{

	class Tracy_Profiler : public Interfaces::IProfiler
	{
	public:
		virtual void StartProfiling() override;
		virtual void StopProfiling() override;

	public:
		virtual ZonePtr CreateZone(FramePtr frame, const std::string& name) const override;

	public:
		void Message(std::string_view text) const override;
		void Message(std::string_view text, uint32_t colour) const override;
		void PlotValue(const std::string& name, int64_t value) override;
		void PlotValue(const std::string& name, double value) override;
		void SetThreadName(const char* name) const override;
		void AppInfo(std::string_view text) const override;

	private:
		const char* GetOrCachePlotName(const std::string& name);

	private:
		std::unordered_map<std::string, std::unique_ptr<char[]>> m_PlotNameCache;
	};

}
// namespace AqualinkAutomate::Profiling
