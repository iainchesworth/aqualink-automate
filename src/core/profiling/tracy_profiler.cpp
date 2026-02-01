#include <algorithm>
#include <cstring>

#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>

#include "profiling/tracy_profiler.h"
#include "profiling/profiling_units/tracy_zone.h"
#include "profiling/types/profiling_types.h"

namespace AqualinkAutomate::Profiling
{

	void Tracy_Profiler::StartProfiling()
	{
	}

	void Tracy_Profiler::StopProfiling()
	{
		tracy::GetProfiler().RequestShutdown();
	}

	ZonePtr Tracy_Profiler::CreateZone(FramePtr frame, const std::string& name) const
	{
		return std::make_unique<TracyZone>(name);
	}

	void Tracy_Profiler::Message(std::string_view text) const
	{
		TracyMessage(text.data(), text.size());
	}

	void Tracy_Profiler::Message(std::string_view text, uint32_t colour) const
	{
		TracyMessageC(text.data(), text.size(), colour);
	}

	void Tracy_Profiler::PlotValue(const std::string& name, int64_t value)
	{
		const char* name_ptr = GetOrCachePlotName(name);
		TracyPlot(name_ptr, value);
	}

	void Tracy_Profiler::PlotValue(const std::string& name, double value)
	{
		const char* name_ptr = GetOrCachePlotName(name);
		TracyPlot(name_ptr, value);
	}

	void Tracy_Profiler::SetThreadName(const char* name) const
	{
		tracy::SetThreadName(name);
	}

	void Tracy_Profiler::AppInfo(std::string_view text) const
	{
		TracyAppInfo(text.data(), text.size());
	}

	const char* Tracy_Profiler::GetOrCachePlotName(const std::string& name)
	{
		if (auto it = m_PlotNameCache.find(name); it != m_PlotNameCache.end())
		{
			return it->second.get();
		}

		auto buffer = std::make_unique<char[]>(name.size() + 1);
		std::memcpy(buffer.get(), name.c_str(), name.size() + 1);
		const char* ptr = buffer.get();
		m_PlotNameCache.emplace(name, std::move(buffer));
		return ptr;
	}

}
// namespace AqualinkAutomate::Profiling
