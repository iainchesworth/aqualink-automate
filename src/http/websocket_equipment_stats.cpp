#include "http/json/json_equipment.h"
#include "http/websocket_equipment_stats.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebSocket_Equipment_Stats::WebSocket_Equipment_Stats(crow::SimpleApp& app, const Kernel::StatisticsHub& statistics_hub) :
		Interfaces::IWebSocket<EQUIPMENTSTATS_WEBSOCKET_URL>(app),
		m_StatisticsHub(statistics_hub),
		m_StatsSlot()
	{
	}

	void WebSocket_Equipment_Stats::OnOpen(Connection& conn)
	{
		m_StatsSlot = m_StatisticsHub.Messages.Signal().connect(
			[this, &conn]() -> void
			{
				LogTrace(Channel::Web, "Publishing updated message count statistics to connected web socket.");

				// Convert JSON to string and send it over the WebSocket connection
				conn.send_text(JSON::GenerateJson_Equipment_Stats(m_StatisticsHub).dump());
			}
		);

		LogTrace(Channel::Web, "Publishing initial message count statistics to connected web socket.");

		// Convert JSON to string and send it over the WebSocket connection
		conn.send_text(JSON::GenerateJson_Equipment_Stats(m_StatisticsHub).dump());
	}

	void WebSocket_Equipment_Stats::OnMessage(Connection& conn, const std::string& data, bool is_binary)
	{
	}

	void WebSocket_Equipment_Stats::OnClose(Connection& conn, const std::string& reason)
	{
		m_StatsSlot.disconnect();
	}

	void WebSocket_Equipment_Stats::OnError(Connection& conn)
	{
		m_StatsSlot.disconnect();
	}

}
// namespace AqualinkAutomate::HTTP
