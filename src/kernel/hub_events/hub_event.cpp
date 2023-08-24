#include "kernel/hub_events/hub_event.h"

namespace AqualinkAutomate::Kernel
{

	Hub_Event::Hub_Event(Hub_EventTypes event_type) :
		m_EventType(event_type)
	{
	}

	Hub_Event::~Hub_Event()
	{
	}

	Hub_EventTypes Hub_Event::Type() const
	{
		return m_EventType;
	}

}
// namespace AqualinkAutomate::Kernel
