#include <format>
#include <functional>

#include <magic_enum/magic_enum.hpp>

#include "http/websocket_event.h"
#include "http/websocket_equipment.h"
#include "logging/logging.h"
#include "profiling/factories/profiling_unit_factory.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebSocket_Equipment::WebSocket_Equipment(Kernel::HubLocator& hub_locator) :
		m_ConfigChangeSlot(),
		m_StatusChangeSlot()
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();
		m_EquipmentHub = hub_locator.Find<Kernel::EquipmentHub>();

		// Connect signals that broadcast messages to all connections
		if (m_DataHub)
		{
			m_ConfigChangeSlot = m_DataHub->ConfigUpdateSignal.connect(
				[this](const std::shared_ptr<Kernel::DataHub_ConfigEvent>& event)
				{
					auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WebSocket_Equipment::on_config_event", std::source_location::current());
					auto payload = HTTP::WebSocket_Event(event).Payload();
					zone->Value(payload.size());
					Broadcast(payload);
				});
		}

		if (m_EquipmentHub)
		{
			m_StatusChangeSlot = m_EquipmentHub->EquipmentStatusChangeSignal.connect(
				[this](const std::shared_ptr<Kernel::EquipmentHub_SystemEvent>& event)
				{
					auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WebSocket_Equipment::on_status_event", std::source_location::current());
					auto payload = HTTP::WebSocket_Event(event).Payload();
					zone->Value(payload.size());
					Broadcast(payload);
				});
		}
	}

	void WebSocket_Equipment::Broadcast(const std::string& payload)
	{
		for (auto& [id, queue] : m_Connections)
		{
			if (queue.size() >= MAX_MESSAGE_QUEUE_SIZE)
			{
				queue.pop_front();
			}
			queue.push_back(payload);
		}
	}

	std::optional<std::string> WebSocket_Equipment::DequeueMessage(ConnectionId connId)
	{
		auto it = m_Connections.find(connId);
		if (it == m_Connections.end() || it->second.empty())
		{
			return std::nullopt;
		}

		auto msg = std::move(it->second.front());
		it->second.pop_front();
		return msg;
	}

	WebSocket_Equipment::ConnectionId WebSocket_Equipment::OnOpen()
	{
		auto connId = m_NextConnectionId++;
		m_Connections[connId] = {};

		LogDebug(Channel::Web, std::format("WebSocket_Equipment: connection {} opened ({} total)", connId, m_Connections.size()));

		if (m_DataHub)
		{
			nlohmann::json state_payload;

			std::string state = "ready";
			if (!m_DataHub->EmulationDisabled && m_DataHub->PoolConfiguration == Kernel::PoolConfigurations::Unknown)
				state = "starting";
			if (m_DataHub->Mode == Kernel::EquipmentMode::Service)
				state = "service_mode";

			state_payload["state"] = state;
			state_payload["pool_configuration"] = std::string{ magic_enum::enum_name(m_DataHub->PoolConfiguration) };
			state_payload["equipment_mode"] = std::string{ magic_enum::enum_name(m_DataHub->Mode) };

			auto payload = WebSocket_Event(WebSocket_EventTypes::SystemStateUpdate, state_payload).Payload();

			auto& queue = m_Connections[connId];
			if (queue.size() >= MAX_MESSAGE_QUEUE_SIZE)
			{
				queue.pop_front();
			}
			queue.push_back(std::move(payload));
		}

		return connId;
	}

	void WebSocket_Equipment::OnMessage(ConnectionId /*connId*/, const boost::beast::flat_buffer& /*buffer*/)
	{
	}

	void WebSocket_Equipment::OnPublish(ConnectionId /*connId*/)
	{
	}

	void WebSocket_Equipment::OnClose(ConnectionId connId)
	{
		m_Connections.erase(connId);
		LogDebug(Channel::Web, std::format("WebSocket_Equipment: connection {} closed ({} remaining)", connId, m_Connections.size()));
	}

	void WebSocket_Equipment::OnError(ConnectionId connId)
	{
		m_Connections.erase(connId);
		LogDebug(Channel::Web, std::format("WebSocket_Equipment: connection {} error ({} remaining)", connId, m_Connections.size()));
	}

}
// namespace AqualinkAutomate::HTTP
