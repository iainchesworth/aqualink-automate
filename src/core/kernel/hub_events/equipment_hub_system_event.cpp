#include "kernel/hub_events/equipment_hub_system_event.h"

namespace AqualinkAutomate::Kernel
{

	EquipmentHub_SystemEvent::EquipmentHub_SystemEvent(Hub_EventTypes event_type) :
		Hub_Event(event_type)
	{
	}

	EquipmentHub_SystemEvent::~EquipmentHub_SystemEvent()
	{
	}

}
// namespace AqualinkAutomate::Kernel
