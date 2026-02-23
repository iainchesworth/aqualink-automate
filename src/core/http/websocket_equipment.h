#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include <boost/signals2.hpp>

#include "interfaces/iwebsocket.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/hub_events/data_hub_config_event_temperature.h"
#include "kernel/hub_events/equipment_hub_system_event_status_change.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENT_WEBSOCKET_URL[] = "/ws/equipment";

	class WebSocket_Equipment : public Interfaces::IWebSocket<EQUIPMENT_WEBSOCKET_URL>
	{
	public:
		WebSocket_Equipment(Kernel::HubLocator& hub_locator);

	public:
		std::optional<std::string> DequeueMessage(ConnectionId connId) override;

	public:
        ConnectionId OnOpen() override;
        void OnMessage(ConnectionId connId, const boost::beast::flat_buffer& buffer) override;
		void OnPublish(ConnectionId connId) override;
        void OnClose(ConnectionId connId) override;
        void OnError(ConnectionId connId) override;

	private:
		void Broadcast(const std::string& payload);

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
		std::shared_ptr<Kernel::EquipmentHub> m_EquipmentHub{ nullptr };
		boost::signals2::connection m_ConfigChangeSlot;
		boost::signals2::connection m_StatusChangeSlot;

		std::mutex m_Mutex;
		std::unordered_map<ConnectionId, std::deque<std::string>> m_Connections;
		ConnectionId m_NextConnectionId{ 1 };

		// Security: Maximum queue size per connection to prevent memory exhaustion
		static constexpr std::size_t MAX_MESSAGE_QUEUE_SIZE = 100;
	};

}
// namespace AqualinkAutomate::HTTP
