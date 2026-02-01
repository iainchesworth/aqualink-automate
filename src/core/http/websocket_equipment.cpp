#include <functional>

#include "http/websocket_event.h"
#include "http/websocket_equipment.h"
#include "logging/logging.h"
#include "profiling/factories/profiling_unit_factory.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebSocket_Equipment::WebSocket_Equipment(Kernel::HubLocator& hub_locator) :
		m_ConfigChangeSlot(),
		m_StatusChangeSlot()
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();
		m_EquipmentHub = hub_locator.Find<Kernel::EquipmentHub>();

		// Connect signals that push messages into the queue
		if (m_DataHub)
		{
			m_ConfigChangeSlot = m_DataHub->ConfigUpdateSignal.connect(
				[this](std::shared_ptr<Kernel::DataHub_ConfigEvent> event)
				{
					auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WS Equipment Config Event", std::source_location::current());
					auto payload = HTTP::WebSocket_Event(event).Payload();
					zone->Value(payload.size());
					m_MessageQueue.push_back(std::move(payload));
				});
		}

		if (m_EquipmentHub)
		{
			m_StatusChangeSlot = m_EquipmentHub->EquipmentStatusChangeSignal.connect(
				[this](std::shared_ptr<Kernel::EquipmentHub_SystemEvent> event)
				{
					auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WS Equipment Status Event", std::source_location::current());
					auto payload = HTTP::WebSocket_Event(event).Payload();
					zone->Value(payload.size());
					m_MessageQueue.push_back(std::move(payload));
				});
		}
	}

	std::optional<std::string> WebSocket_Equipment::DequeueMessage()
	{
		if (m_MessageQueue.empty())
		{
			return std::nullopt;
		}

		auto msg = std::move(m_MessageQueue.front());
		m_MessageQueue.pop_front();
		return msg;
	}

	void WebSocket_Equipment::OnOpen()
	{
	}

	void WebSocket_Equipment::OnMessage(const boost::beast::flat_buffer& buffer)
	{
	}

	void WebSocket_Equipment::OnPublish()
	{
	}

	void WebSocket_Equipment::OnClose()
	{
	}

	void WebSocket_Equipment::OnError()
	{
	}

}
// namespace AqualinkAutomate::HTTP
