#include "http/websocket_event.h"
#include "http/websocket_equipment.h"
#include "kernel/event_temperature.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebSocket_Equipment::WebSocket_Equipment(crow::SimpleApp& app, const Kernel::DataHub& data_hub) :
		Interfaces::IWebSocket<EQUIPMENT_WEBSOCKET_URL>(app),
		m_DataHub(data_hub),
		m_TemperatureSlot(),
		m_Connection(std::nullopt)
	{
	}

	void WebSocket_Equipment::OnOpen(Connection& conn)
	{
		m_Connection = conn;

		m_TemperatureSlot = m_DataHub.ConfigUpdateSignal.connect(std::bind(&WebSocket_Equipment::HandleEvent_TemperatureUpdate, this, std::placeholders::_1));
	}

	void WebSocket_Equipment::OnMessage(Connection& conn, const std::string& data, bool is_binary)
	{
	}

	void WebSocket_Equipment::OnClose(Connection& conn, const std::string& reason)
	{
		m_TemperatureSlot.disconnect();
		m_Connection = std::nullopt;
	}

	void WebSocket_Equipment::OnError(Connection& conn)
	{
		m_TemperatureSlot.disconnect();
		m_Connection = std::nullopt;
	}

	void WebSocket_Equipment::HandleEvent_TemperatureUpdate(std::shared_ptr<Kernel::DataHub_Event> config_update_event)
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
