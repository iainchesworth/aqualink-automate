#pragma once

#include <boost/uuid/uuid.hpp>
#include <nlohmann/json.hpp>

#include "kernel/data_hub_events/data_hub_event.h"
#include "kernel/data_hub_events/data_hub_eventtypes.h"

namespace AqualinkAutomate::Kernel
{

	class DataHub_SystemEvent : public DataHub_Event
	{
	public:
		DataHub_SystemEvent(DataHub_EventTypes event_type);
		virtual ~DataHub_SystemEvent();
	};

}
// namespace AqualinkAutomate::Kernel
