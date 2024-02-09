#pragma once

#include <string>

#include <boost/uuid/uuid.hpp>

namespace AqualinkAutomate::Interfaces
{

	class IProfilingUnit
	{
	public:
		IProfilingUnit(std::string name);
		virtual ~IProfilingUnit() = default;

	public:
		virtual void Start() const = 0;
		virtual void Mark() const = 0;
		virtual void End() const = 0;

	public:
		boost::uuids::uuid UUID() const;
		const std::string& Name() const;

	private:
		const std::string m_Name;
		boost::uuids::uuid m_UUID;
	};

}
// namespace AqualinkAutomate::Interfaces
