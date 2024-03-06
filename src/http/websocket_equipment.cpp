#include <functional>

#include <boost/cobalt/race.hpp>

#include "coroutines/awaitable_signal.h"
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

	boost::cobalt::generator<std::string> WebSocket_Equipment::MessageGenerator()
	{
		while (!co_await boost::cobalt::this_coro::cancelled)
		{
			auto res = co_await boost::cobalt::race(
				Coroutines::AwaitSignal<std::shared_ptr<Kernel::DataHub_ConfigEvent>>(m_DataHub->ConfigUpdateSignal),
				Coroutines::AwaitSignal<std::shared_ptr<Kernel::EquipmentHub_SystemEvent>>(m_EquipmentHub->EquipmentStatusChangeSignal)
			);

			struct SignalVisitor
			{
				std::string operator()(std::shared_ptr<Kernel::DataHub_ConfigEvent> config_update_event)
				{
					return HTTP::WebSocket_Event(config_update_event).Payload();
				}
				std::string operator()(std::shared_ptr<Kernel::EquipmentHub_SystemEvent> system_update_event)
				{
					return HTTP::WebSocket_Event(system_update_event).Payload();
				}
			};

			boost::variant2::visit(SignalVisitor(), res);
		}

		co_return std::string{};
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
