#include <format>
#include <string>

#include "profiling/uprof_profiler.h"
#include "profiling/types/profiling_types.h"

namespace AqualinkAutomate::Profiling
{

	void UProf_Profiler::StartProfiling()
	{
		amdProfileResume();
	}

	void UProf_Profiler::StopProfiling()
	{
		amdProfilePause();
	}

	ZonePtr UProf_Profiler::CreateZone(FramePtr frame, const std::string& name) const
	{
#if defined(UProf_ACTIVITY_LOGGER_ENABLED)
		return std::make_unique<UProfZone>(name);
#else
		return std::make_unique<Zone>(name);
#endif
	}

	void UProf_Profiler::Message(std::string_view text) const
	{
#if defined(UProf_ACTIVITY_LOGGER_ENABLED)
		std::string text_str(text);
		amdtBeginMarker(text_str.c_str(), "Messages");
		amdtEndMarker();
#endif
	}

	void UProf_Profiler::Message(std::string_view text, uint32_t colour) const
	{
#if defined(UProf_ACTIVITY_LOGGER_ENABLED)
		// uProf markers don't support colour
		std::string text_str(text);
		amdtBeginMarker(text_str.c_str(), "Messages");
		amdtEndMarker();
#endif
	}

	void UProf_Profiler::PlotValue(const std::string& name, int64_t value)
	{
#if defined(UProf_ACTIVITY_LOGGER_ENABLED)
		auto formatted = std::format("{}: {}", name, value);
		amdtBeginMarker(formatted.c_str(), "Counters");
		amdtEndMarker();
#endif
	}

	void UProf_Profiler::PlotValue(const std::string& name, double value)
	{
#if defined(UProf_ACTIVITY_LOGGER_ENABLED)
		auto formatted = std::format("{}: {:.2f}", name, value);
		amdtBeginMarker(formatted.c_str(), "Counters");
		amdtEndMarker();
#endif
	}

	void UProf_Profiler::SetThreadName(const char*) const
	{
		// Not supported by uProf
	}

	void UProf_Profiler::AppInfo(std::string_view text) const
	{
#if defined(UProf_ACTIVITY_LOGGER_ENABLED)
		std::string text_str(text);
		amdtBeginMarker(text_str.c_str(), "AppInfo");
		amdtEndMarker();
#endif
	}

	void UProf_Profiler::EmitFrameMark(const char* name) const
	{
#if defined(UProf_ACTIVITY_LOGGER_ENABLED)
		amdtBeginMarker(name, "Frames");
		amdtEndMarker();
#endif
	}

}
// namespace AqualinkAutomate::Profiling
