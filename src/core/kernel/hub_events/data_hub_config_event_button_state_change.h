#pragma once

#include <string>

#include <boost/uuid/uuid.hpp>
#include <nlohmann/json.hpp>

#include "kernel/hub_events/data_hub_config_event.h"

namespace AqualinkAutomate::Kernel
{

	class DataHub_ConfigEvent_ButtonStateChange : public DataHub_ConfigEvent
	{
	public:
		DataHub_ConfigEvent_ButtonStateChange(const boost::uuids::uuid& button_id, std::string_view status);
		virtual ~DataHub_ConfigEvent_ButtonStateChange();

	public:
		boost::uuids::uuid ButtonId() const;
		std::string_view Status() const;

	public:
		virtual boost::uuids::uuid Id() const override;
		virtual nlohmann::json ToJSON() const override;

	private:
		boost::uuids::uuid m_ButtonId;
		std::string m_Status;
	};

}
// namespace AqualinkAutomate::Kernel
