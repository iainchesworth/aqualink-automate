#pragma once

#include <string>

#include <boost/uuid/uuid.hpp>

namespace AqualinkAutomate::Interfaces
{

	class IProfilingUnit
	{
	public:
		IProfilingUnit(const std::string& name);
		virtual ~IProfilingUnit() = default;

	public:
		virtual void Start() = 0;
		virtual void Mark() = 0;
		virtual void End() = 0;

	public:
		boost::uuids::uuid UUID() const;
		const std::string& Name() const;

	private:
		const std::string m_Name;
		boost::uuids::uuid m_UUID;
	};

}
// namespace AqualinkAutomate::Interfaces
