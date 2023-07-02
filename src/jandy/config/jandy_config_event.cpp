#include "jandy/config/jandy_config_event.h"

namespace AqualinkAutomate::Config
{

	JandyConfig_Event::JandyConfig_Event(JandyConfig_EventTypes event_type) :
		m_EventType(event_type)
	{
	}

	JandyConfig_EventTypes JandyConfig_Event::Type() const
	{
		return m_EventType;
	}

}
// namespace AqualinkAutomate::Config
