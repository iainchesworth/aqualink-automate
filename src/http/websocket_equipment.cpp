#include <functional>

#include "http/websocket_event.h"
#include "http/websocket_equipment.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebSocket_Equipment::WebSocket_Equipment(HTTP::Server& http_server, Kernel::HubLocator& hub_locator) :
		Interfaces::IWebSocket<EQUIPMENT_WEBSOCKET_URL>(http_server),
		Interfaces::IShareableRoute(),
		m_TemperatureSlot(),
		m_StatusChangeSlot()
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();
		m_EquipmentHub = hub_locator.Find<Kernel::EquipmentHub>();
	}

	void WebSocket_Equipment::OnOpen(HTTP::Request& req)
	{
		m_TemperatureSlot = m_DataHub->ConfigUpdateSignal.connect(std::bind(&WebSocket_Equipment::HandleEvent_DataHubConfigUpdate, this, std::placeholders::_1));
		m_StatusChangeSlot = m_EquipmentHub->EquipmentStatusChangeSignal.connect(std::bind(&WebSocket_Equipment::HandleEvent_DataHubSystemUpdate, this, std::placeholders::_1));
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

	void WebSocket_Equipment::HandleEvent_DataHubConfigUpdate(std::shared_ptr<Kernel::DataHub_ConfigEvent> config_update_event)
	{
		if (nullptr == config_update_event)
		{
			LogDebug(Channel::Web, "Received an invalid Kernel::DataHub_ConfigEvent; config_update_event -> nullptr");
		}
		else
		{
			BroadcastMessage_AsText(HTTP::WebSocket_Event(config_update_event).Payload());
		}
	}

	void WebSocket_Equipment::HandleEvent_DataHubSystemUpdate(std::shared_ptr<Kernel::EquipmentHub_SystemEvent> system_update_event)
	{
		if (nullptr == system_update_event)
		{
			LogDebug(Channel::Web, "Received an invalid Kernel::EquipmentHub_SystemEvent; system_update_event -> nullptr");
		}
		else
		{
			BroadcastMessage_AsText(HTTP::WebSocket_Event(system_update_event).Payload());
		}
	}

}
// namespace AqualinkAutomate::HTTP
