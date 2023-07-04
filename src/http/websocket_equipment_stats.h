#pragma once

#include <string>

#include <boost/signals2.hpp>
#include <crow/app.h>

#include "interfaces/iwebsocket.h"
#include "kernel/statistics_hub.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char EQUIPMENTSTATS_WEBSOCKET_URL[] = "/ws/equipment/stats";

	class WebSocket_Equipment_Stats: public Interfaces::IWebSocket<EQUIPMENTSTATS_WEBSOCKET_URL>
	{
	public:
		WebSocket_Equipment_Stats(crow::SimpleApp& app, const Kernel::StatisticsHub& statistics_hub);

	private:
		virtual void OnOpen(Connection& conn) override;
		virtual void OnMessage(Connection& conn, const std::string& data, bool is_binary) override;
		virtual void OnClose(Connection& conn, const std::string& reason) override;
		virtual void OnError(Connection& conn) override;

	private:
		const Kernel::StatisticsHub& m_StatisticsHub;
		boost::signals2::connection m_StatsSlot;
	};


}
// namespace AqualinkAutomate::HTTP
