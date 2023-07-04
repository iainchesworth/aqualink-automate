#include "kernel/data_hub_event.h"

namespace AqualinkAutomate::Kernel
{

	DataHub_Event::DataHub_Event(DataHub_EventTypes event_type) :
		m_EventType(event_type)
	{
	}

	DataHub_EventTypes DataHub_Event::Type() const
	{
		return m_EventType;
	}

}
// namespace AqualinkAutomate::Kernel
