#include "profiling/profiling_units/tracy_zone.h"
#include "profiling/profiling_units/tracy_zone_datamap.h"

namespace AqualinkAutomate::Profiling
{

	TracyZone::TracyZone(std::string_view name, const std::source_location& src_loc, UnitColours colour) :
		Zone(name)
	{
		std::string name_str(name);
		const auto* src_loc_data = TracyZone_DataMap::Instance().FindOrInsert(name_str, src_loc, colour);
		if (src_loc_data)
		{
			m_TSZ = std::make_unique<tracy::ScopedZone>(src_loc_data);
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

	void TracyZone::Text(std::string_view text) const
	{
		if (m_TSZ)
		{
			m_TSZ->Text(text.data(), text.size());
		}
	}

	void TracyZone::Value(uint64_t value) const
	{
		if (m_TSZ)
		{
			m_TSZ->Value(value);
		}
	}

}
// namespace AqualinkAutomate::Profiling
