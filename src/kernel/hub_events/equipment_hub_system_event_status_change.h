#pragma once

#include <boost/uuid/uuid.hpp>
#include <nlohmann/json.hpp>

#include "interfaces/istatus.h"
#include "kernel/hub_events/equipment_hub_system_event.h"

namespace AqualinkAutomate::Kernel
{

	class EquipmentHub_SystemEvent_StatusChange : public EquipmentHub_SystemEvent
	{
	public:
		EquipmentHub_SystemEvent_StatusChange(const Interfaces::IStatus& status);
		virtual ~EquipmentHub_SystemEvent_StatusChange();

	public:
		virtual boost::uuids::uuid Id() const override;
		virtual nlohmann::json ToJSON() const override;

	private:
		nlohmann::json m_JSON_Payload;
	};

}
// namespace AqualinkAutomate::Kernel
