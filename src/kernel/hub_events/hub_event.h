#pragma once

#include <boost/uuid/uuid.hpp>
#include <nlohmann/json.hpp>

#include "kernel/hub_events/hub_eventtypes.h"

namespace AqualinkAutomate::Kernel
{

	class Hub_Event
	{
	public:
		Hub_Event(Hub_EventTypes event_type);
		virtual ~Hub_Event();

	public:
		Hub_EventTypes Type() const;

	public:
		virtual boost::uuids::uuid Id() const = 0;
		virtual nlohmann::json ToJSON() const = 0;

	private:
		Hub_EventTypes m_EventType;
	};

}
// namespace AqualinkAutomate::Kernel
