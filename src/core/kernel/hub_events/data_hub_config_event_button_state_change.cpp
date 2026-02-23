#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "kernel/hub_events/data_hub_config_event_button_state_change.h"

namespace AqualinkAutomate::Kernel
{

	DataHub_ConfigEvent_ButtonStateChange::DataHub_ConfigEvent_ButtonStateChange(const boost::uuids::uuid& button_id, std::string_view status) :
		DataHub_ConfigEvent(Hub_EventTypes::ButtonStateChange),
		m_ButtonId(button_id),
		m_Status(status)
	{
	}

	DataHub_ConfigEvent_ButtonStateChange::DataHub_ConfigEvent_ButtonStateChange(const boost::uuids::uuid& button_id, std::string_view status, std::string_view label) :
		DataHub_ConfigEvent(Hub_EventTypes::ButtonStateChange),
		m_ButtonId(button_id),
		m_Status(status),
		m_Label(label)
	{
	}


	boost::uuids::uuid DataHub_ConfigEvent_ButtonStateChange::ButtonId() const
	{
		return m_ButtonId;
	}

	std::string_view DataHub_ConfigEvent_ButtonStateChange::Status() const
	{
		return m_Status;
	}

	std::string_view DataHub_ConfigEvent_ButtonStateChange::Label() const
	{
		return m_Label;
	}

	boost::uuids::uuid DataHub_ConfigEvent_ButtonStateChange::Id() const
	{
		static boost::uuids::uuid id{ boost::uuids::random_generator()() };
		return id;
	}

	nlohmann::json DataHub_ConfigEvent_ButtonStateChange::ToJSON() const
	{
		nlohmann::json j;

		j["button_id"] = boost::uuids::to_string(m_ButtonId);
		j["status"] = m_Status;

		if (!m_Label.empty())
		{
			j["label"] = m_Label;
		}

		return j;
	}

}
// namespace AqualinkAutomate::Kernel
