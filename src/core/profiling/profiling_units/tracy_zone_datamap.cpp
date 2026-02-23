#include <algorithm>
#include <cstring>

#include "profiling/profiling_units/tracy_zone_datamap.h"

namespace AqualinkAutomate::Profiling
{

	TracyZone_DataMap& TracyZone_DataMap::Instance()
	{
		static TracyZone_DataMap instance;
		return instance;
	}

	const tracy::SourceLocationData* TracyZone_DataMap::FindOrInsert(
		const std::string& name,
		const std::source_location& src_loc,
		UnitColours colour)
	{
		std::lock_guard lock(m_Mutex);

		if (auto it = m_Map.find(name); it != m_Map.end())
		{
			return &std::get<tracy::SourceLocationData>(it->second);
		}

		DataTuple tracy_data;

		std::get<std::string>(tracy_data) = name;

		// Intentionally leaked: Tracy holds raw pointers to these buffers
		// in SourceLocationData and they must remain valid for program lifetime.
		auto* name_ptr = new char[name.size() + 1];
		std::memcpy(name_ptr, name.c_str(), name.size() + 1);
		std::get<char*>(tracy_data) = name_ptr;

		std::get<std::source_location>(tracy_data) = src_loc;
		std::get<tracy::SourceLocationData>(tracy_data) =
		{
			name_ptr,
			std::get<std::source_location>(tracy_data).function_name(),
			std::get<std::source_location>(tracy_data).file_name(),
			std::get<std::source_location>(tracy_data).line(),
			static_cast<uint32_t>(colour)
		};

		auto [it, was_inserted] = m_Map.emplace(name, std::move(tracy_data));
		if (was_inserted)
		{
			return &std::get<tracy::SourceLocationData>(it->second);
		}

		return nullptr;
	}

}
// namespace AqualinkAutomate::Profiling
