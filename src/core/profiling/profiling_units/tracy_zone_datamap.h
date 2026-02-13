#pragma once

#include <mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>

#include <boost/container_hash/hash.hpp>
#include <tracy/Tracy.hpp>

#include "profiling/profiling_units/unit_colours.h"

namespace AqualinkAutomate::Profiling
{

	class TracyZone_DataMap
	{
	public:
		using DataTuple = std::tuple<std::string, char*,
		                             std::source_location, tracy::SourceLocationData>;

	public:
		static TracyZone_DataMap& Instance();

		// Thread-safe find-or-insert. Returns pointer to SourceLocationData.
		const tracy::SourceLocationData* FindOrInsert(
			const std::string& name,
			const std::source_location& src_loc,
			UnitColours colour);

	public:
		TracyZone_DataMap(const TracyZone_DataMap&) = delete;
		TracyZone_DataMap& operator=(const TracyZone_DataMap&) = delete;

	private:
		TracyZone_DataMap() = default;

		using DataMap = std::unordered_map<std::string, DataTuple, boost::hash<std::string>>;
		DataMap m_Map;
		std::mutex m_Mutex;
	};

}
// namespace AqualinkAutomate::Profiling
