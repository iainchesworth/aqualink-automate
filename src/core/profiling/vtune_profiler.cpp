#include <string>

#include "profiling/vtune_profiler.h"
#include "profiling/types/profiling_types.h"

namespace AqualinkAutomate::Profiling
{

	static __itt_domain* GetVTuneDomain()
	{
		static auto* domain = __itt_domain_create("AqualinkAutomate");
		return domain;
	}

	VTune_Profiler::VTune_Profiler()
		: m_Domain(GetVTuneDomain())
	{
	}

	void VTune_Profiler::StartProfiling()
	{
		__itt_resume();
	}

	void VTune_Profiler::StopProfiling()
	{
		__itt_pause();
	}

	ZonePtr VTune_Profiler::CreateZone(FramePtr frame, const std::string& name) const
	{
		return std::make_unique<VTuneZone>(name);
	}

	void VTune_Profiler::Message(std::string_view text) const
	{
		std::string text_str(text);
		auto* handle = GetOrCreateStringHandle(text_str);
		__itt_marker(m_Domain, __itt_null, handle, __itt_marker_scope_global);
	}

	void VTune_Profiler::Message(std::string_view text, uint32_t colour) const
	{
		// ITT markers don't support colour; emit the same marker
		std::string text_str(text);
		auto* handle = GetOrCreateStringHandle(text_str);
		__itt_marker(m_Domain, __itt_null, handle, __itt_marker_scope_global);
	}

	void VTune_Profiler::PlotValue(const std::string& name, int64_t value)
	{
		auto counter = GetOrCreateCounter(name);
		__itt_counter_set_value(counter, &value);
	}

	void VTune_Profiler::PlotValue(const std::string& name, double value)
	{
		auto counter = GetOrCreateCounter(name);
		__itt_counter_set_value(counter, &value);
	}

	void VTune_Profiler::SetThreadName(const char* name) const
	{
		__itt_thread_set_name(name);
	}

	void VTune_Profiler::AppInfo(std::string_view text) const
	{
		static auto* key = __itt_string_handle_create("AppInfo");
		__itt_metadata_str_add(m_Domain, __itt_null, key, text.data(), text.size());
	}

	void VTune_Profiler::EmitFrameMark(const char* name) const
	{
		__itt_frame_begin_v3(m_Domain, NULL);
		__itt_frame_end_v3(m_Domain, NULL);
	}

	__itt_counter VTune_Profiler::GetOrCreateCounter(const std::string& name)
	{
		if (auto it = m_CounterCache.find(name); it != m_CounterCache.end())
		{
			return it->second;
		}

		auto counter = __itt_counter_create(name.c_str(), "AqualinkAutomate");
		m_CounterCache.emplace(name, counter);
		return counter;
	}

	__itt_string_handle* VTune_Profiler::GetOrCreateStringHandle(const std::string& name) const
	{
		if (auto it = m_StringHandleCache.find(name); it != m_StringHandleCache.end())
		{
			return it->second;
		}

		auto* handle = __itt_string_handle_create(name.c_str());
		m_StringHandleCache.emplace(name, handle);
		return handle;
	}

}
// namespace AqualinkAutomate::Profiling
