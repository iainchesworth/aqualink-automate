#pragma once

#include <boost/uuid/uuid.hpp>

namespace AqualinkAutomate::Config
{

	enum class JandyConfig_EventTypes
	{
		Temperature
	};

	class JandyConfig_Event
	{
	public:
		JandyConfig_Event(JandyConfig_EventTypes event_type);

	public:
		JandyConfig_EventTypes Type() const;

	public:
		virtual boost::uuids::uuid Id() const = 0;

	private:
		JandyConfig_EventTypes m_EventType;
	};

}
// namespace AqualinkAutomate::Config
