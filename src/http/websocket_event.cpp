#include <algorithm>

#include <magic_enum.hpp>

#include "http/websocket_event.h"
#include "logging/logging.h"
#include "utility/case_insensitive_comparision.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	const std::string_view WebSocket_Event::WS_JSON_TYPE_FIELD{ "type" };
	const std::string_view WebSocket_Event::WS_JSON_PAYLOAD_FIELD{ "payload" };

	WebSocket_Event::WebSocket_Event(const HTTP::WebSocket_EventTypes& event_type, const nlohmann::json& payload) :
		m_EventType(event_type),
		m_EventPayload()
	{
		m_EventPayload[WS_JSON_TYPE_FIELD] = magic_enum::enum_name(event_type);
		m_EventPayload[WS_JSON_PAYLOAD_FIELD] = payload;
	}

	WebSocket_Event::WebSocket_Event(std::shared_ptr<Kernel::DataHub_Event> config_event) :
		m_EventType(WebSocket_EventTypes::Unknown)
	{
		if (nullptr == config_event)
		{
			LogDebug(Channel::Web, "Invalid event type; cannot process DataHub_Event type and payload.");
		}
		else
		{
			switch (config_event->Type())
			{
			case Kernel::DataHub_EventTypes::Chemistry:
				this->operator=(std::dynamic_pointer_cast<Kernel::DataHub_Event_Chemistry>(config_event));
				break;

			case Kernel::DataHub_EventTypes::Temperature:
				this->operator=(std::dynamic_pointer_cast<Kernel::DataHub_Event_Temperature>(config_event));
				break;

			default:
				LogDebug(Channel::Web, "Unknown event type; cannot process DataHub_Event type and payload.");
			}
		}
	}

	WebSocket_Event::WebSocket_Event(std::shared_ptr<Kernel::DataHub_Event_Chemistry> chem_config_event) :
		m_EventType(WebSocket_EventTypes::Unknown)
	{
		this->operator=(chem_config_event);
	}

	WebSocket_Event::WebSocket_Event(std::shared_ptr<Kernel::DataHub_Event_Temperature> temp_config_event) :
		m_EventType(WebSocket_EventTypes::Unknown)
	{
		this->operator=(temp_config_event);
	}

	WebSocket_Event& WebSocket_Event::operator=(std::shared_ptr<Kernel::DataHub_Event_Chemistry> chem_config_event)
	{
		if (nullptr == chem_config_event)
		{
			LogDebug(Channel::Web, "Invalid DataHub_Event_Chemistry; cannot process type and payload.");
		}
		else
		{
			m_EventType = WebSocket_EventTypes::ChemistryUpdate;
			m_EventPayload[WS_JSON_TYPE_FIELD] = magic_enum::enum_name(m_EventType);
			m_EventPayload[WS_JSON_PAYLOAD_FIELD] = chem_config_event->ToJSON();
		}

		return *this;
	}

	WebSocket_Event& WebSocket_Event::operator=(std::shared_ptr<Kernel::DataHub_Event_Temperature> temp_config_event)
	{
		if (nullptr == temp_config_event)
		{
			LogDebug(Channel::Web, "Invalid DataHub_Event_Temperature; cannot process type and payload.");
		}
		else
		{
			m_EventType = WebSocket_EventTypes::TemperatureUpdate;
			m_EventPayload[WS_JSON_TYPE_FIELD] = magic_enum::enum_name(m_EventType);
			m_EventPayload[WS_JSON_PAYLOAD_FIELD] = temp_config_event->ToJSON();
		}

		return *this;
	}

	WebSocket_EventTypes WebSocket_Event::Type() const
	{
		return m_EventType;
	}

	std::string WebSocket_Event::Payload() const
	{
		return m_EventPayload.dump();
	}

	std::string WebSocket_Event::operator()() const
	{
		return Payload();
	}

	std::optional<WebSocket_Event> WebSocket_Event::ConvertFromString(const std::string& event_payload)
	{
		std::optional<WebSocket_Event> return_event{std::nullopt};

		auto parsed_event{nlohmann::json::parse(event_payload, nullptr, false, false)};

		if (!parsed_event.contains(WS_JSON_TYPE_FIELD))
		{
			LogDebug(Channel::Web, "Invalid websocket event; does not contain type field");
		}
		else if (!parsed_event.contains(WS_JSON_PAYLOAD_FIELD))
		{
			LogDebug(Channel::Web, "Invalid websocket event; does not contain payload field");
		}
		else if (!parsed_event[WS_JSON_TYPE_FIELD].is_string())
		{
			LogDebug(Channel::Web, "Invalid websocket event; type field is invalid type");
		}
		else
		{
			const std::string ws_event_type_as_string = parsed_event[WS_JSON_TYPE_FIELD];

			auto ws_event_type = magic_enum::enum_cast<WebSocket_EventTypes>(ws_event_type_as_string, Utility::case_insensitive_comparision).value_or(WebSocket_EventTypes::Unknown);
			auto ws_event_payload = parsed_event[WS_JSON_PAYLOAD_FIELD];

			return WebSocket_Event(ws_event_type, ws_event_payload);
		}

		return return_event;
	}

	std::optional<WebSocket_Event> WebSocket_Event::ConvertFromStringView(const std::string_view& event_payload)
	{
		return ConvertFromString(std::string(event_payload.data(), event_payload.size()));
	}
}
// namespace AqualinkAutomate::HTTP
