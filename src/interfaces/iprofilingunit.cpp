#include <boost/uuid/uuid_generators.hpp>

#include "interfaces/iprofilingunit.h"

namespace AqualinkAutomate::Interfaces
{

	IProfilingUnit::IProfilingUnit(std::string name) :
		m_Name(std::move(name)),
		m_UUID(boost::uuids::random_generator()())
	{
	}

	boost::uuids::uuid IProfilingUnit::UUID() const
	{
		return m_UUID;
	}

	const std::string& IProfilingUnit::Name() const
	{
		return m_Name;
	}

}
// namespace AqualinkAutomate::Interfaces
