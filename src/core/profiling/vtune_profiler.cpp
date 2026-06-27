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
		// Close any frame opened by EmitFrameMark so the trace is well-formed.
		if (m_FrameOpen)
		{
			__itt_frame_end_v3(m_Domain, nullptr);
			m_FrameOpen = false;
		}
		__itt_pause();
	}

	void VTune_Profiler::Resume()
	{
		__itt_resume();
	}

	void VTune_Profiler::Pause()
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
		auto counter = GetOrCreateCounter(name, __itt_metadata_s64);
		__itt_counter_set_value(counter, &value);
	}

	void VTune_Profiler::PlotValue(const std::string& name, double value)
	{
		auto counter = GetOrCreateCounter(name, __itt_metadata_double);
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
		// A frame mark is a BOUNDARY: end the previous frame (if any) and begin
		// the next, so the interval between consecutive marks is recorded as one
		// frame. (The earlier begin+end pair recorded zero-width frames.)
		if (m_FrameOpen)
		{
			__itt_frame_end_v3(m_Domain, nullptr);
		}
		__itt_frame_begin_v3(m_Domain, nullptr);
		m_FrameOpen = true;
	}

	__itt_counter VTune_Profiler::GetOrCreateCounter(const std::string& name, __itt_metadata_type type)
	{
		if (auto it = m_CounterCache.find(name); it != m_CounterCache.end())
		{
			return it->second;
		}

		// Create a typed counter so float/integer values render correctly; an
		// untyped counter defaults to u64 and would misinterpret doubles and
		// negative values. A given plot name is always one value type in practice.
		auto counter = __itt_counter_create_typed(name.c_str(), "AqualinkAutomate", type);
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
