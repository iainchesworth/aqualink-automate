#include <algorithm>

#include "profiling/profiling_units/tracy_zone.h"
#include "profiling/profiling_units/tracy_zone_datamap.h"

namespace AqualinkAutomate::Profiling
{

	TracyZone::TracyZone(const std::string name, const boost::source_location& src_loc, UnitColours colour) :
		Zone(name)
	{
		if (auto iter = TracyZone_DataMap::Instance().find(name); TracyZone_DataMap::Instance().end() != iter)
		{
			m_TSZ = new tracy::ScopedZone(&(std::get<tracy::SourceLocationData>(iter->second)));
		}
		else
		{
			TracyZone_DataMap::DataTuple tracy_data;

			std::get<std::string>(tracy_data) = name;

			std::get<char*>(tracy_data) = new char[name.size()+1];
			auto name_ptr = std::get<char*>(tracy_data);
			std::fill(&name_ptr[0], &name_ptr[name.size()+1], 0);
			std::copy(name.cbegin(), name.cend(), name_ptr);

			std::get<boost::source_location>(tracy_data) = src_loc;
			std::get<tracy::SourceLocationData>(tracy_data) =
			{
				std::get<char*>(tracy_data),
				std::get<boost::source_location>(tracy_data).function_name(),
				std::get<boost::source_location>(tracy_data).file_name(),
				std::get<boost::source_location>(tracy_data).line(),
				static_cast<uint32_t>(colour)
			};

			if (auto [it, was_inserted] = TracyZone_DataMap::Instance().emplace(name, tracy_data); was_inserted)
			{
				m_TSZ = new tracy::ScopedZone(&(std::get<tracy::SourceLocationData>(it->second)));
			}
			else
			{
				///FIXME
			}
		}
	}

	TracyZone::~TracyZone()
	{
		if (nullptr != m_TSZ)
		{
			delete m_TSZ;
			m_TSZ = nullptr;
		}
	}

	inline void TracyZone::Start() const
	{
	}

	inline void TracyZone::Mark() const
	{
	}

	inline void TracyZone::End() const
	{
	}

}
// namespace AqualinkAutomate::Profiling
