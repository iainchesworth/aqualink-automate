#pragma once

#include <memory>

#include <boost/signals2.hpp>

#include "interfaces/ishareableroute.h"
#include "interfaces/iwebsocket.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/hub_events/data_hub_config_event_temperature.h"
#include "kernel/hub_events/equipment_hub_system_event_status_change.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENT_WEBSOCKET_URL[] = "/ws/equipment";

	class WebSocket_Equipment : public Interfaces::IWebSocket<EQUIPMENT_WEBSOCKET_URL>, public Interfaces::IShareableRoute
	{
	public:
		WebSocket_Equipment(HTTP::Server& http_server, Kernel::HubLocator& hub_locator);

	private:
		virtual void OnOpen(HTTP::Request& req) override;
		virtual void OnMessage(HTTP::Request& req) override;
		virtual void OnClose(HTTP::Request& req) override;
		virtual void OnError(HTTP::Request& req) override;

	private:
		void HandleEvent_DataHubConfigUpdate(std::shared_ptr<Kernel::DataHub_ConfigEvent> config_update_event);
		void HandleEvent_DataHubSystemUpdate(std::shared_ptr<Kernel::EquipmentHub_SystemEvent> system_update_event);

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
		std::shared_ptr<Kernel::EquipmentHub> m_EquipmentHub{ nullptr };
		boost::signals2::connection m_TemperatureSlot;
		boost::signals2::connection m_StatusChangeSlot;
	};


}
// namespace AqualinkAutomate::HTTP
