#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <tracy/Tracy.hpp>

#include "profiling/profiling_units/tracy_frame.h"

namespace AqualinkAutomate::Profiling
{

	// Tracy requires that frame name pointers are:
	//  1. Unique per name (pointer identity is used to identify frame sets)
	//  2. Valid for the entire program lifetime (never freed)
	// This static cache satisfies both requirements.
	const char* TracyFrame::GetOrCacheFrameName(std::string_view name)
	{
		static std::unordered_map<std::string, std::unique_ptr<char[]>> s_Cache;
		static std::mutex s_Mutex;

		std::lock_guard lock(s_Mutex);

		std::string key(name);
		if (auto it = s_Cache.find(key); it != s_Cache.end())
		{
			return it->second.get();
		}

		auto buffer = std::make_unique<char[]>(name.size() + 1);
		std::memcpy(buffer.get(), name.data(), name.size());
		buffer[name.size()] = '\0';
		const char* ptr = buffer.get();
		s_Cache.emplace(std::move(key), std::move(buffer));
		return ptr;
	}

	TracyFrame::TracyFrame(std::string_view name, const std::source_location& src_loc, UnitColours colour) :
		Profiling::Frame(name, src_loc, colour),
		m_NamePtr(GetOrCacheFrameName(name))
	{
	}

	void TracyFrame::Start() const
	{
		FrameMarkStart(m_NamePtr);
	}

	void TracyFrame::Mark() const
	{
		FrameMarkNamed(m_NamePtr);
	}

	void TracyFrame::End() const
	{
		FrameMarkEnd(m_NamePtr);
	}

}
// namespace AqualinkAutomate::Profiling
