#pragma once

#include <boost/uuid/uuid.hpp>
#include <nlohmann/json.hpp>

#include "kernel/data_hub_events/data_hub_eventtypes.h"

namespace AqualinkAutomate::Kernel
{

	class DataHub_Event
	{
	public:
		DataHub_Event(DataHub_EventTypes event_type);
		virtual ~DataHub_Event();

	public:
		DataHub_EventTypes Type() const;

	public:
		virtual boost::uuids::uuid Id() const = 0;
		virtual nlohmann::json ToJSON() const = 0;

	private:
		DataHub_EventTypes m_EventType;
	};

}
// namespace AqualinkAutomate::Kernel
