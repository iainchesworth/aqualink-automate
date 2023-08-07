#pragma once

#include <boost/signals2.hpp>

#include "interfaces/ishareableroute.h"
#include "interfaces/iwebsocket.h"
#include "kernel/data_hub.h"
#include "kernel/data_hub_events/data_hub_event_temperature.h"
#include "kernel/data_hub_events/data_hub_system_event_status_change.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENT_WEBSOCKET_URL[] = "/ws/equipment";

	class WebSocket_Equipment : public Interfaces::IWebSocket<EQUIPMENT_WEBSOCKET_URL>, public Interfaces::IShareableRoute
	{
	public:
		WebSocket_Equipment(HTTP::Server& http_server, const Kernel::DataHub& data_hub);

	private:
		virtual void OnOpen(HTTP::Request& req) override;
		virtual void OnMessage(HTTP::Request& req) override;
		virtual void OnClose(HTTP::Request& req) override;
		virtual void OnError(HTTP::Request& req) override;

	private:
		void HandleEvent_DataHubConfigUpdate(std::shared_ptr<Kernel::DataHub_Event> config_update_event);
		void HandleEvent_DataHubSystemUpdate(std::shared_ptr<Kernel::DataHub_SystemEvent> system_update_event);

	private:
		const Kernel::DataHub& m_DataHub;
		boost::signals2::connection m_TemperatureSlot;
		boost::signals2::connection m_StatusChangeSlot;
	};


}
// namespace AqualinkAutomate::HTTP
