#include "profiling/profiling_units/uprof_zone.h"

namespace AqualinkAutomate::Profiling
{

	UProfZone::UProfZone(std::string_view name, const std::source_location& src_loc, UnitColours colour) :
		Zone(name, src_loc, colour),
		m_NameStr(name)
	{
		amdtBeginMarker(m_NameStr.c_str(), "AqualinkAutomate");
	}

	UProfZone::~UProfZone()
	{
		amdtEndMarker();
	}

	void UProfZone::Start() const
	{
	}

	void UProfZone::Mark() const
	{
	}

	void UProfZone::End() const
	{
	}

	void UProfZone::Text(std::string_view) const
	{
		// ActivityLogger markers don't support text annotations
	}

	void UProfZone::Value(uint64_t) const
	{
		// ActivityLogger markers don't support value annotations
	}

}
// namespace AqualinkAutomate::Profiling
