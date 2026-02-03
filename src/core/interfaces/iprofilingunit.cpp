#include "interfaces/iprofilingunit.h"

namespace AqualinkAutomate::Interfaces
{

	IProfilingUnit::IProfilingUnit(std::string_view name) :
		m_Name(name)
	{
	}

	std::string_view IProfilingUnit::Name() const
	{
		return m_Name;
	}

	void IProfilingUnit::Text(std::string_view) const
	{
	}

	void IProfilingUnit::Value(uint64_t) const
	{
	}

}
// namespace AqualinkAutomate::Interfaces
