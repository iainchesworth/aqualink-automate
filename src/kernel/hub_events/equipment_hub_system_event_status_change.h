#pragma once

#include <boost/uuid/uuid.hpp>
#include <nlohmann/json.hpp>

#include "kernel/hub_events/equipment_hub_system_event.h"

namespace AqualinkAutomate::Kernel
{

	class EquipmentHub_SystemEvent_StatusChange : public EquipmentHub_SystemEvent
	{
	public:
		EquipmentHub_SystemEvent_StatusChange();
		~EquipmentHub_SystemEvent_StatusChange();

	public:
		virtual boost::uuids::uuid Id() const override;
		virtual nlohmann::json ToJSON() const override;
	};

}
// namespace AqualinkAutomate::Kernel
