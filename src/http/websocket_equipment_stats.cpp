#include <cstdint>

#include "coroutines/awaitable_signal.h"
#include "http/json/json_equipment.h"
#include "http/websocket_equipment_stats.h"
#include "http/websocket_event.h"
#include "http/websocket_event_types.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebSocket_Equipment_Stats::WebSocket_Equipment_Stats(Kernel::HubLocator& hub_locator) :
		Interfaces::IWebSocket<EQUIPMENTSTATS_WEBSOCKET_URL>(),
		m_StatsSlot()
	{
		m_StatisticsHub = hub_locator.Find<Kernel::StatisticsHub>();
	}

	boost::cobalt::generator<std::string> WebSocket_Equipment_Stats::MessageGenerator()
	{
		while (!co_await boost::cobalt::this_coro::cancelled)
		{
			co_await Coroutines::AwaitSignal<uint64_t>(m_StatisticsHub->MessageCounts.Signal());
			
			co_yield HTTP::WebSocket_Event(HTTP::WebSocket_EventTypes::StatisticsUpdate, JSON::GenerateJson_Equipment_Stats(m_StatisticsHub)).Payload();
		}

		co_return std::string{};
	}

	void WebSocket_Equipment_Stats::OnOpen()
	{
	}

	void WebSocket_Equipment_Stats::OnMessage(const boost::beast::flat_buffer& buffer)
	{
	}

	void WebSocket_Equipment_Stats::OnPublish()
	{
	}

	void WebSocket_Equipment_Stats::OnClose()
	{
	}

	void WebSocket_Equipment_Stats::OnError()
	{
	}

}
// namespace AqualinkAutomate::HTTP
