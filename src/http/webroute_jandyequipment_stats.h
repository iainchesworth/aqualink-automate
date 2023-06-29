#pragma once

#include <string>

#include <boost/signals2.hpp>
#include <crow/app.h>

#include "interfaces/iwebsocket.h"
#include "jandy/equipment/jandy_equipment.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char EQUIPMENTSTATS_ROUTE_URL[] = "/api/equipment/stats";

	class WebRoute_JandyEquipment_Stats: public Interfaces::IWebSocket<EQUIPMENTSTATS_ROUTE_URL>
	{
	public:
		WebRoute_JandyEquipment_Stats(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment);

	public:
		void OnOpen(Connection& conn);
		void OnMessage(Connection& conn, const std::string& data, bool is_binary);
		void OnClose(Connection& conn, const std::string& reason);
		void OnError(Connection& conn);

	private:
		const Equipment::JandyEquipment& m_JandyEquipment;
		boost::signals2::connection m_StatsSlot;
	};


}
// namespace AqualinkAutomate::HTTP
