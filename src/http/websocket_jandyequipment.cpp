#include "http/websocket_event.h"
#include "http/websocket_jandyequipment.h"
#include "jandy/config/jandy_config_event_temperature.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebSocket_JandyEquipment::WebSocket_JandyEquipment(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment) :
		Interfaces::IWebSocket<EQUIPMENT_WEBSOCKET_URL>(app),
		m_JandyEquipment(jandy_equipment),
		m_TemperatureSlot(),
		m_Connection(std::nullopt)
	{
	}

	void WebSocket_JandyEquipment::OnOpen(Connection& conn)
	{
		m_Connection = conn;

		m_TemperatureSlot = m_JandyEquipment.Config().ConfigUpdateSignal.connect(std::bind(&WebSocket_JandyEquipment::HandleEvent_TemperatureUpdate, this, std::placeholders::_1));
	}

	void WebSocket_JandyEquipment::OnMessage(Connection& conn, const std::string& data, bool is_binary)
	{
	}

	void WebSocket_JandyEquipment::OnClose(Connection& conn, const std::string& reason)
	{
		m_TemperatureSlot.disconnect();
		m_Connection = std::nullopt;
	}

	void WebSocket_JandyEquipment::OnError(Connection& conn)
	{
		m_TemperatureSlot.disconnect();
		m_Connection = std::nullopt;
	}

	void WebSocket_JandyEquipment::HandleEvent_TemperatureUpdate(std::shared_ptr<Config::JandyConfig_Event> config_update_event)
	{
		if (nullptr == config_update_event)
		{
			///FIXME
		}
		else if (!m_Connection.has_value())
		{
			///FIXME
		}
		else
		{
			m_Connection.value().get().send_text(HTTP::WebSocket_Event(config_update_event).Payload());
		}
	}

}
// namespace AqualinkAutomate::HTTP
