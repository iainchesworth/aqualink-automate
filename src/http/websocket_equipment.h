#pragma once

#include <any>
#include <functional>
#include <optional>
#include <string>

#include <boost/signals2.hpp>

#include "interfaces/iwebsocket.h"
#include "kernel/data_hub.h"
#include "kernel/data_hub_event_temperature.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENT_WEBSOCKET_URL[] = "/ws/equipment";

	class WebSocket_Equipment : public Interfaces::IWebSocket<EQUIPMENT_WEBSOCKET_URL>
	{
	public:
		WebSocket_Equipment(HTTP::Server& http_server, const Kernel::DataHub& data_hub);

	private:
		virtual void OnOpen(HTTP::Request& req) override;
		virtual void OnMessage(HTTP::Request& req) override;
		virtual void OnClose(HTTP::Request& req) override;
		virtual void OnError(HTTP::Request& req) override;

	private:
		void HandleEvent_DataHubUpdate(std::shared_ptr<Kernel::DataHub_Event> config_update_event);

	private:
		const Kernel::DataHub& m_DataHub;
		boost::signals2::connection m_TemperatureSlot;
	};


}
// namespace AqualinkAutomate::HTTP
