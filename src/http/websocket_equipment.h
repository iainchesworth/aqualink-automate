#pragma once

#include <functional>
#include <optional>
#include <string>

#include <boost/signals2.hpp>
#include <crow/app.h>

#include "interfaces/iwebsocket.h"
#include "kernel/data_hub.h"
#include "kernel/data_hub_event_temperature.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char EQUIPMENT_WEBSOCKET_URL[] = "/ws/equipment";

	class WebSocket_Equipment : public Interfaces::IWebSocket<EQUIPMENT_WEBSOCKET_URL>
	{
	public:
		WebSocket_Equipment(crow::SimpleApp& app, const Kernel::DataHub& data_hub);

	private:
		virtual void OnOpen(Connection& conn) override;
		virtual void OnMessage(Connection& conn, const std::string& data, bool is_binary) override;
		virtual void OnClose(Connection& conn, const std::string& reason) override;
		virtual void OnError(Connection& conn) override;

	private:
		void HandleEvent_DataHubUpdate(std::shared_ptr<Kernel::DataHub_Event> config_update_event);

	private:
		const Kernel::DataHub& m_DataHub;
		boost::signals2::connection m_TemperatureSlot;
		std::optional<std::reference_wrapper<Connection>> m_Connection;
	};


}
// namespace AqualinkAutomate::HTTP
