#pragma once

#include <string>
#include <tuple>
#include <unordered_map>

#include <boost/assert/source_location.hpp>
#include <boost/container_hash/hash.hpp>
#include <tracy/Tracy.hpp>

namespace AqualinkAutomate::Profiling
{

	class TracyZone_DataMap
	{
	public:
		using DataTuple = std::tuple<std::string, char*, boost::source_location, tracy::SourceLocationData>;
		using DataMap = std::unordered_map<std::string, DataTuple, boost::hash<std::string>>;

	public:
		static DataMap& Instance();

	public:
		TracyZone_DataMap(const TracyZone_DataMap&) = delete;
		TracyZone_DataMap& operator=(const TracyZone_DataMap&) = delete;

	private:
		TracyZone_DataMap() 
		{
		}
	};

}
// namespace AqualinkAutomate::Profiling
