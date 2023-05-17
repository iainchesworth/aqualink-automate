#include <crow/mustache.h>
#include <magic_enum.hpp>
#include <nlohmann/json.hpp>

#include "http/webroute_jandyequipment_stats.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_JandyEquipment_Stats::WebRoute_JandyEquipment_Stats(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment) :
		Interfaces::IWebSocket<EQUIPMENTSTATS_ROUTE_URL>(app),
		m_JandyEquipment(jandy_equipment)
	{
	}

	void WebRoute_JandyEquipment_Stats::OnOpen(Connection& conn)
	{
		// Convert JSON to string and send it over the WebSocket connection
		conn.send_text(GenerateStats());
	}

	void WebRoute_JandyEquipment_Stats::OnMessage(Connection& conn, const std::string& data, bool is_binary)
	{
	}

	void WebRoute_JandyEquipment_Stats::OnClose(Connection& conn, const std::string& reason)
	{
	}

	void WebRoute_JandyEquipment_Stats::OnError(Connection& conn)
	{
	}

	std::string WebRoute_JandyEquipment_Stats::GenerateStats() const
	{
		nlohmann::json message_stats;

		for (auto [id, count] : m_JandyEquipment.m_MessageStats)
		{
			nlohmann::json stat;
			stat["id"] = magic_enum::enum_name(id);
			stat["count"] = count;
			message_stats.push_back(stat);
		}

		return message_stats.dump();
	}

}
// namespace AqualinkAutomate::HTTP
