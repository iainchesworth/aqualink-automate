#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "http/websocket_event_types.h"
#include "kernel/data_hub_events/data_hub_config_event.h"
#include "kernel/data_hub_events/data_hub_config_event_chemistry.h"
#include "kernel/data_hub_events/data_hub_config_event_temperature.h"
#include "kernel/data_hub_events/data_hub_system_event.h"
#include "kernel/data_hub_events/data_hub_system_event_status_change.h"

namespace AqualinkAutomate::HTTP
{

	class WebSocket_Event
	{
		static const std::string_view WS_JSON_TYPE_FIELD;
		static const std::string_view WS_JSON_PAYLOAD_FIELD;

	public:
		WebSocket_Event(const WebSocket_EventTypes& event_type, const nlohmann::json& payload);

	public:
		WebSocket_Event(std::shared_ptr<Kernel::DataHub_ConfigEvent> config_event);
		WebSocket_Event(std::shared_ptr<Kernel::DataHub_ConfigEvent_Chemistry> chem_config_event);
		WebSocket_Event(std::shared_ptr<Kernel::DataHub_ConfigEvent_Temperature> temp_config_event);
		WebSocket_Event(std::shared_ptr<Kernel::DataHub_SystemEvent> system_event);
		WebSocket_Event(std::shared_ptr<Kernel::DataHub_SystemEvent_StatusChange> status_system_event);

	public:
		WebSocket_Event& operator=(std::shared_ptr<Kernel::DataHub_ConfigEvent_Chemistry> chem_config_event);
		WebSocket_Event& operator=(std::shared_ptr<Kernel::DataHub_ConfigEvent_Temperature> temp_config_event);
		WebSocket_Event& operator=(std::shared_ptr<Kernel::DataHub_SystemEvent_StatusChange> status_system_event);

	public:
		WebSocket_EventTypes Type() const;
		std::string Payload() const;

	public:
		std::string operator()() const;

	private:
		WebSocket_EventTypes m_EventType;
		nlohmann::json m_EventPayload;

	public:
		static std::optional<WebSocket_Event> ConvertFromString(const std::string& event_payload);
		static std::optional<WebSocket_Event> ConvertFromStringView(const std::string_view& event_payload);
	};

}
// namespace AqualinkAutomate::HTTP
