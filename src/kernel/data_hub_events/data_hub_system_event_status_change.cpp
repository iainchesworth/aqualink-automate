#include <boost/uuid/uuid_generators.hpp>
#include <magic_enum.hpp>

#include "kernel/data_hub_events/data_hub_system_event_status_change.h"
#include "localisation/translations_and_units_formatter.h"

namespace AqualinkAutomate::Kernel
{

	DataHub_SystemEvent_StatusChange::DataHub_SystemEvent_StatusChange() :
		DataHub_SystemEvent(DataHub_EventTypes::ServiceStatus)
	{
	}

	DataHub_SystemEvent_StatusChange::~DataHub_SystemEvent_StatusChange()
	{
	}

	boost::uuids::uuid DataHub_SystemEvent_StatusChange::Id() const
	{
		static boost::uuids::uuid id{ boost::uuids::random_generator()() };
		return id;
	}

	nlohmann::json DataHub_SystemEvent_StatusChange::ToJSON() const
	{
		nlohmann::json j;

		j["service_status"] = "Unknown";

		return j;
	}

}
// namespace AqualinkAutomate::Kernel
