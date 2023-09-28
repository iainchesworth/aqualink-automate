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

	void WebSocket_Equipment_Stats::OnOpen(HTTP::Request& req)
	{
		m_StatsSlot = m_StatisticsHub->MessageCounts.Signal().connect(
			[this]() -> void
			{
				LogTrace(Channel::Web, "Publishing updated message count statistics to connected web socket.");

				// Convert JSON to string and send it over the WebSocket connection
				HTTP::WebSocket_Event ws_statistics_update(HTTP::WebSocket_EventTypes::StatisticsUpdate, JSON::GenerateJson_Equipment_Stats(m_StatisticsHub));
				BroadcastMessage_AsText(ws_statistics_update.Payload());
			}
		);

		LogTrace(Channel::Web, "Publishing initial message count statistics to connected web socket.");

		// Convert JSON to string and send it over the WebSocket connection
		HTTP::WebSocket_Event ws_statistics_update(HTTP::WebSocket_EventTypes::StatisticsUpdate, JSON::GenerateJson_Equipment_Stats(m_StatisticsHub));
		///FIXME PublishMessage_AsText(req.get_conn<cinatra::NonSSL>(), ws_statistics_update.Payload());
	}

	void WebSocket_Equipment_Stats::OnMessage(HTTP::Request& reqy)
	{
	}

	void WebSocket_Equipment_Stats::OnClose(HTTP::Request& req)
	{
		m_StatsSlot.disconnect();
	}

	void WebSocket_Equipment_Stats::OnError(HTTP::Request& req)
	{
		m_StatsSlot.disconnect();
	}

}
// namespace AqualinkAutomate::HTTP
