#pragma once

#include <boost/uuid/uuid.hpp>
#include <nlohmann/json.hpp>

#include "kernel/service_statuses.h"
#include "kernel/data_hub_events/data_hub_system_event.h"

namespace AqualinkAutomate::Kernel
{

	class DataHub_SystemEvent_StatusChange : public DataHub_SystemEvent
	{
	public:
		DataHub_SystemEvent_StatusChange(Kernel::ServiceStatuses service_status);
		~DataHub_SystemEvent_StatusChange();

	public:
		Kernel::ServiceStatuses ServiceStatus() const;

	public:
		virtual boost::uuids::uuid Id() const override;
		virtual nlohmann::json ToJSON() const override;

	private:
		Kernel::ServiceStatuses m_ServiceStatus;
	};

}
// namespace AqualinkAutomate::Kernel
