#include <string>

#include "profiling/profiling_units/vtune_zone.h"

namespace AqualinkAutomate::Profiling
{

	static __itt_domain* GetVTuneDomain()
	{
		static auto* domain = __itt_domain_create("AqualinkAutomate");
		return domain;
	}

	VTuneZone::VTuneZone(std::string_view name, const std::source_location& src_loc, UnitColours colour) :
		Zone(name, src_loc, colour),
		m_Domain(GetVTuneDomain()),
		m_Name(__itt_string_handle_create(std::string(name).c_str()))
	{
		__itt_task_begin(m_Domain, __itt_null, __itt_null, m_Name);
	}

	VTuneZone::~VTuneZone()
	{
		__itt_task_end(m_Domain);
	}

	void VTuneZone::Start() const
	{
	}

	void VTuneZone::Mark() const
	{
	}

	void VTuneZone::End() const
	{
	}

	void VTuneZone::Text(std::string_view text) const
	{
		static auto* key = __itt_string_handle_create("text");
		__itt_metadata_str_add(m_Domain, __itt_null, key, text.data(), text.size());
	}

	void VTuneZone::Value(uint64_t value) const
	{
		static auto* key = __itt_string_handle_create("value");
		__itt_metadata_add(m_Domain, __itt_null, key, __itt_metadata_u64, 1, &value);
	}

}
// namespace AqualinkAutomate::Profiling
