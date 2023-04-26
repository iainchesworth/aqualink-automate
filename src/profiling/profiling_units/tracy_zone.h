#pragma once

#include <source_location>
#include <unordered_map>

#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid.hpp>
#include <tracy/Tracy.hpp>

#include "profiling/profiling_units/unit_colours.h"
#include "profiling/profiling_units/zone.h"

namespace AqualinkAutomate::Profiling
{

	class TracyZone : public Profiling::Zone
	{
	public:
		TracyZone(const std::string name, const std::source_location& src_loc = std::source_location::current(), UnitColours colour = UnitColours::NotSpecified);
		virtual ~TracyZone();

	public:
		virtual void Start() override;
		virtual void Mark() override;
		virtual void End() override;

	private:
		using TracyDataTuple = std::tuple<std::string, char*, std::source_location, tracy::SourceLocationData>;
		using TracyDataMap = std::unordered_map<std::string, TracyDataTuple, boost::hash<std::string>>;
		static TracyDataMap m_TracyDataMap;

	private:
		tracy::ScopedZone* m_TSZ{ nullptr };
	};

}
// namespace AqualinkAutomate::Profiling
