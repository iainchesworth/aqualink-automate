#pragma once

#include <string>

#include <boost/signals2.hpp>

#include "interfaces/iwebsocket.h"
#include "kernel/statistics_hub.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char EQUIPMENTSTATS_WEBSOCKET_URL[] = "/ws/equipment/stats";

	class WebSocket_Equipment_Stats: public Interfaces::IWebSocket<EQUIPMENTSTATS_WEBSOCKET_URL>
	{
	public:
		WebSocket_Equipment_Stats(HTTP::Server& http_server, const Kernel::StatisticsHub& statistics_hub);

	private:
		virtual void OnOpen(HTTP::Request& req) override;
		virtual void OnMessage(HTTP::Request& req) override;
		virtual void OnClose(HTTP::Request& req) override;
		virtual void OnError(HTTP::Request& req) override;

	private:
		const Kernel::StatisticsHub& m_StatisticsHub;
		boost::signals2::connection m_StatsSlot;
	};


}
// namespace AqualinkAutomate::HTTP
