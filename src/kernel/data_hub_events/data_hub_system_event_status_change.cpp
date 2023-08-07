#include <boost/uuid/uuid_generators.hpp>
#include <magic_enum.hpp>

#include "kernel/data_hub_events/data_hub_system_event_status_change.h"
#include "localisation/translations_and_units_formatter.h"

namespace AqualinkAutomate::Kernel
{

	DataHub_SystemEvent_StatusChange::DataHub_SystemEvent_StatusChange(Kernel::ServiceStatuses service_status) :
		DataHub_SystemEvent(DataHub_EventTypes::ServiceStatus),
		m_ServiceStatus(service_status)
	{
	}

	DataHub_SystemEvent_StatusChange::~DataHub_SystemEvent_StatusChange()
	{
	}

	Kernel::ServiceStatuses DataHub_SystemEvent_StatusChange::ServiceStatus() const
	{
		return m_ServiceStatus;
	}

	boost::uuids::uuid DataHub_SystemEvent_StatusChange::Id() const
	{
		static boost::uuids::uuid id{ boost::uuids::random_generator()() };
		return id;
	}

	nlohmann::json DataHub_SystemEvent_StatusChange::ToJSON() const
	{
		nlohmann::json j;

		j["service_status"] = magic_enum::enum_name(m_ServiceStatus);

		return j;
	}

}
// namespace AqualinkAutomate::Kernel
