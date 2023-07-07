#include "http/websocket_event.h"
#include "http/websocket_equipment.h"
#include "kernel/data_hub_event_temperature.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebSocket_Equipment::WebSocket_Equipment(HTTP::Server& http_server, const Kernel::DataHub& data_hub) :
		Interfaces::IWebSocket<EQUIPMENT_WEBSOCKET_URL>(http_server),
		m_DataHub(data_hub),
		m_TemperatureSlot()
	{
	}

	void WebSocket_Equipment::OnOpen(HTTP::Request& req)
	{
		m_TemperatureSlot = m_DataHub.ConfigUpdateSignal.connect(std::bind(&WebSocket_Equipment::HandleEvent_DataHubUpdate, this, std::placeholders::_1));
	}

	void WebSocket_Equipment::OnMessage(HTTP::Request& req)
	{
	}

	void WebSocket_Equipment::OnClose(HTTP::Request& req)
	{
		m_TemperatureSlot.disconnect();
	}

	void WebSocket_Equipment::OnError(HTTP::Request& req)
	{
		m_TemperatureSlot.disconnect();
	}

	void WebSocket_Equipment::HandleEvent_DataHubUpdate(std::shared_ptr<Kernel::DataHub_Event> config_update_event)
	{
		if (nullptr == config_update_event)
		{
			///FIXME
		}
		else
		{
			PublishMessage_AsText(HTTP::WebSocket_Event(config_update_event).Payload());
		}
	}

}
// namespace AqualinkAutomate::HTTP
