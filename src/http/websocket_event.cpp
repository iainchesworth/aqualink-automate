#include "http/websocket_event.h"

namespace AqualinkAutomate::HTTP
{

	const std::string_view WebSocket_Event::WS_JSON_TYPE_FIELD{ "type" };
	const std::string_view WebSocket_Event::WS_JSON_PAYLOAD_FIELD{ "payload" };

	WebSocket_Event::WebSocket_Event(std::shared_ptr<Config::JandyConfig_Event> config_event) :
		m_EventType(WebSocket_EventTypes::Unknown)
	{
		if (nullptr == config_event)
		{
			///FIXME
		}
		else
		{
			switch (config_event->Type())
			{
			case Config::JandyConfig_EventTypes::Temperature:
				this->operator=(std::dynamic_pointer_cast<Config::JandyConfig_Event_Temperature>(config_event));
				break;
			}
		}
	}

	WebSocket_Event::WebSocket_Event(std::shared_ptr<Config::JandyConfig_Event_Temperature> temp_config_event) :
		m_EventType(WebSocket_EventTypes::Unknown)
	{
		this->operator=(temp_config_event);
	}

	WebSocket_Event& WebSocket_Event::operator=(std::shared_ptr<Config::JandyConfig_Event_Temperature> temp_config_event)
	{
		if (nullptr == temp_config_event)
		{
			///FIXME
		}
		else
		{
			m_EventType = WebSocket_EventTypes::TemperatureUpdate;
			m_EventPayload[WS_JSON_TYPE_FIELD] = magic_enum::enum_name(m_EventType);
			m_EventPayload[WS_JSON_PAYLOAD_FIELD] = *temp_config_event;
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

}
// namespace AqualinkAutomate::HTTP
