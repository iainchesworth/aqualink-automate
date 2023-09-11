#include <boost/uuid/uuid_generators.hpp>
#include <magic_enum.hpp>

#include "kernel/hub_events/equipment_hub_system_event_status_change.h"
#include "localisation/translations_and_units_formatter.h"

namespace AqualinkAutomate::Kernel
{

	EquipmentHub_SystemEvent_StatusChange::EquipmentHub_SystemEvent_StatusChange(const Interfaces::IStatus& status) :
		EquipmentHub_SystemEvent(Hub_EventTypes::ServiceStatus),
		m_JSON_Payload(status)
	{
	}

	EquipmentHub_SystemEvent_StatusChange::~EquipmentHub_SystemEvent_StatusChange()
	{
	}

	boost::uuids::uuid EquipmentHub_SystemEvent_StatusChange::Id() const
	{
		static boost::uuids::uuid id{ boost::uuids::random_generator()() };
		return id;
	}

	nlohmann::json EquipmentHub_SystemEvent_StatusChange::ToJSON() const
	{
		return m_JSON_Payload;
	}

}
// namespace AqualinkAutomate::Kernel
