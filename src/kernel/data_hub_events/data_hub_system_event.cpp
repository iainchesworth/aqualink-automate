#include "kernel/data_hub_events/data_hub_system_event.h"

namespace AqualinkAutomate::Kernel
{

	DataHub_SystemEvent::DataHub_SystemEvent(DataHub_EventTypes event_type) :
		DataHub_Event(event_type)
	{
	}

	DataHub_SystemEvent::~DataHub_SystemEvent()
	{
	}

}
// namespace AqualinkAutomate::Kernel
