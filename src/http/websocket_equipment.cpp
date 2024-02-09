#include <functional>

#include "http/websocket_event.h"
#include "http/websocket_equipment.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebSocket_Equipment::WebSocket_Equipment(Kernel::HubLocator& hub_locator) :
		m_ConfigChangeSlot(),
		m_StatusChangeSlot()
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();
		m_EquipmentHub = hub_locator.Find<Kernel::EquipmentHub>();
	}

	void WebSocket_Equipment::OnOpen()
	{
		m_ConfigChangeSlot = m_DataHub->ConfigUpdateSignal.connect([this](auto&& PH1) { HandleEvent_DataHubConfigUpdate(std::forward<decltype(PH1)>(PH1)); });
		m_StatusChangeSlot = m_EquipmentHub->EquipmentStatusChangeSignal.connect([this](auto&& PH1) { HandleEvent_DataHubSystemUpdate(std::forward<decltype(PH1)>(PH1)); });
	}

	void WebSocket_Equipment::OnMessage(const boost::beast::flat_buffer& buffer)
	{
	}

	void WebSocket_Equipment::OnClose()
	{
		m_ConfigChangeSlot.disconnect();
	}

	void WebSocket_Equipment::OnError()
	{
		m_ConfigChangeSlot.disconnect();
	}

	void WebSocket_Equipment::HandleEvent_DataHubConfigUpdate(std::shared_ptr<Kernel::DataHub_ConfigEvent> config_update_event)
	{
		if (nullptr == config_update_event)
		{
			LogDebug(Channel::Web, "Received an invalid Kernel::DataHub_ConfigEvent; config_update_event -> nullptr");
		}
		else
		{
			BroadcastMessage(HTTP::WebSocket_Event(config_update_event).Payload());
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
			BroadcastMessage(HTTP::WebSocket_Event(system_update_event).Payload());
		}
	}

}
// namespace AqualinkAutomate::HTTP
