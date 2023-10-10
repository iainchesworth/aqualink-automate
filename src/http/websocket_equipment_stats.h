#pragma once

#include <memory>
#include <string>

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

	private:
		virtual void OnOpen() override;
		virtual void OnMessage(const boost::beast::flat_buffer& buffer) override;
		virtual void OnClose() override;
		virtual void OnError() override;

	private:
		std::shared_ptr<Kernel::StatisticsHub> m_StatisticsHub{ nullptr };
		boost::signals2::connection m_StatsSlot;
	};

}
// namespace AqualinkAutomate::HTTP
