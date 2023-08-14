#pragma once

#include <boost/uuid/uuid.hpp>
#include <nlohmann/json.hpp>

#include "kernel/data_hub_events/data_hub_system_event.h"

namespace AqualinkAutomate::Kernel
{

	class DataHub_SystemEvent_StatusChange : public DataHub_SystemEvent
	{
	public:
		DataHub_SystemEvent_StatusChange();
		~DataHub_SystemEvent_StatusChange();

	public:
		virtual boost::uuids::uuid Id() const override;
		virtual nlohmann::json ToJSON() const override;
	};

}
// namespace AqualinkAutomate::Kernel
