#include "http/webroute_jandyequipment_stats.h"
#include "http/json/json_jandy_equipment.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebRoute_JandyEquipment_Stats::WebRoute_JandyEquipment_Stats(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment) :
		Interfaces::IWebSocket<EQUIPMENTSTATS_ROUTE_URL>(app),
		m_JandyEquipment(jandy_equipment),
		m_StatsSlot()
	{
	}

	void WebRoute_JandyEquipment_Stats::OnOpen(Connection& conn)
	{
		m_StatsSlot = m_JandyEquipment.MessageStats().Signal().connect(
			[this, &conn]() -> void
			{
				LogDebug(Channel::Web, "Publishing updated message count statistics to connected web socket.");

				// Convert JSON to string and send it over the WebSocket connection
				conn.send_text(JSON::GenerateJson_JandyEquipment_Stats(m_JandyEquipment).dump());
			}
		);

		LogDebug(Channel::Web, "Publishing initial message count statistics to connected web socket.");

		// Convert JSON to string and send it over the WebSocket connection
		conn.send_text(JSON::GenerateJson_JandyEquipment_Stats(m_JandyEquipment).dump());
	}

	void WebRoute_JandyEquipment_Stats::OnMessage(Connection& conn, const std::string& data, bool is_binary)
	{
	}

	void WebRoute_JandyEquipment_Stats::OnClose(Connection& conn, const std::string& reason)
	{
		m_StatsSlot.disconnect();
	}

	void WebRoute_JandyEquipment_Stats::OnError(Connection& conn)
	{
		m_StatsSlot.disconnect();
	}

}
// namespace AqualinkAutomate::HTTP
