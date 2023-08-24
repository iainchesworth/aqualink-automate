#include <boost/uuid/uuid_generators.hpp>
#include <magic_enum.hpp>

#include "kernel/hub_events/equipment_hub_system_event_status_change.h"
#include "localisation/translations_and_units_formatter.h"

namespace AqualinkAutomate::Kernel
{

	EquipmentHub_SystemEvent_StatusChange::EquipmentHub_SystemEvent_StatusChange() :
		EquipmentHub_SystemEvent(Hub_EventTypes::ServiceStatus)
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
		nlohmann::json j;

		j["service_status"] = "Unknown";

		return j;
	}

}
// namespace AqualinkAutomate::Kernel
