#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include <boost/signals2.hpp>

#include "interfaces/iwebsocket.h"
#include "kernel/hub_locator.h"
#include "kernel/statistics_hub.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENTSTATS_WEBSOCKET_URL[] = "/ws/equipment/stats";

	class WebSocket_Equipment_Stats: public Interfaces::IWebSocket<EQUIPMENTSTATS_WEBSOCKET_URL>
	{
	public:
		WebSocket_Equipment_Stats(Kernel::HubLocator& hub_locator);

	public:
		std::optional<std::string> DequeueMessage(ConnectionId connId) override;

	public:
		ConnectionId OnOpen() override;
		void OnMessage(ConnectionId connId, const boost::beast::flat_buffer& buffer) override;
		void OnPublish(ConnectionId connId) override;
		void OnClose(ConnectionId connId) override;
		void OnError(ConnectionId connId) override;

	private:
		struct ConnectionState
		{
			std::deque<std::string> queue;
			bool dirty{ false };
		};

		std::shared_ptr<Kernel::StatisticsHub> m_StatisticsHub{ nullptr };
		boost::signals2::connection m_StatsSlot;

		std::mutex m_Mutex;
		std::unordered_map<ConnectionId, ConnectionState> m_Connections;
		ConnectionId m_NextConnectionId{ 1 };
	};

}
// namespace AqualinkAutomate::HTTP
