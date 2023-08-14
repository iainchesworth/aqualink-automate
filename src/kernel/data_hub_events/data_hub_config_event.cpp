#include "kernel/data_hub_events/data_hub_config_event.h"

namespace AqualinkAutomate::Kernel
{

	DataHub_ConfigEvent::DataHub_ConfigEvent(DataHub_EventTypes event_type) :
		DataHub_Event(event_type)
	{
	}

	DataHub_ConfigEvent::~DataHub_ConfigEvent()
	{
	}

}
// namespace AqualinkAutomate::Kernel
