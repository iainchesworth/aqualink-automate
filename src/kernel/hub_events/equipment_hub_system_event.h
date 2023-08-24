#pragma once

#include <boost/uuid/uuid.hpp>
#include <nlohmann/json.hpp>

#include "kernel/hub_events/hub_event.h"
#include "kernel/hub_events/hub_eventtypes.h"

namespace AqualinkAutomate::Kernel
{

	class EquipmentHub_SystemEvent : public Hub_Event
	{
	public:
		EquipmentHub_SystemEvent(Hub_EventTypes event_type);
		virtual ~EquipmentHub_SystemEvent();
	};

}
// namespace AqualinkAutomate::Kernel
