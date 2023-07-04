#pragma once

#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "http/websocket_event_types.h"
#include "kernel/data_hub_event.h"
#include "kernel/data_hub_event_chemistry.h"
#include "kernel/data_hub_event_temperature.h"

namespace AqualinkAutomate::HTTP
{

	class WebSocket_Event
	{
		static const std::string_view WS_JSON_TYPE_FIELD;
		static const std::string_view WS_JSON_PAYLOAD_FIELD;

	public:
		WebSocket_Event(std::shared_ptr<Kernel::DataHub_Event> config_event);
		WebSocket_Event(std::shared_ptr<Kernel::DataHub_Event_Chemistry> chem_config_event);
		WebSocket_Event(std::shared_ptr<Kernel::DataHub_Event_Temperature> temp_config_event);

	public:
		WebSocket_Event& operator=(std::shared_ptr<Kernel::DataHub_Event_Chemistry> chem_config_event);
		WebSocket_Event& operator=(std::shared_ptr<Kernel::DataHub_Event_Temperature> temp_config_event);

	public:
		WebSocket_EventTypes Type() const;
		std::string Payload() const;

	public:
		std::string operator()() const;

	private:
		WebSocket_EventTypes m_EventType;
		nlohmann::json m_EventPayload;
	};

}
// namespace AqualinkAutomate::HTTP
