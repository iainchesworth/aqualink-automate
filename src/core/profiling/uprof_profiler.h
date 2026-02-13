#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include <AMDProfileController.h>

#include "interfaces/iprofiler.h"
#include "profiling/profiling_units/domain.h"
#include "profiling/profiling_units/frame.h"
#include "profiling/profiling_units/zone.h"

#if defined(UProf_ACTIVITY_LOGGER_ENABLED)
#include <AMDTActivityLogger.h>
#include "profiling/profiling_units/uprof_frame.h"
#include "profiling/profiling_units/uprof_zone.h"
#endif

namespace AqualinkAutomate::Profiling
{

	class UProf_Profiler : public Interfaces::IProfiler
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
		void EmitFrameMark(const char* name) const override;
	};

}
// namespace AqualinkAutomate::Profiling
