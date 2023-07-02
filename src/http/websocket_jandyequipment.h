#pragma once

#include <functional>
#include <optional>
#include <string>

#include <boost/signals2.hpp>
#include <crow/app.h>

#include "interfaces/iwebsocket.h"
#include "jandy/config/jandy_config_event_temperature.h"
#include "jandy/equipment/jandy_equipment.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char EQUIPMENT_WEBSOCKET_URL[] = "/ws/equipment";

	class WebSocket_JandyEquipment : public Interfaces::IWebSocket<EQUIPMENT_WEBSOCKET_URL>
	{
	public:
		WebSocket_JandyEquipment(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment);

	private:
		virtual void OnOpen(Connection& conn) override;
		virtual void OnMessage(Connection& conn, const std::string& data, bool is_binary) override;
		virtual void OnClose(Connection& conn, const std::string& reason) override;
		virtual void OnError(Connection& conn) override;

	private:
		void HandleEvent_TemperatureUpdate(std::shared_ptr<Config::JandyConfig_Event> config_update_event);

	private:
		const Equipment::JandyEquipment& m_JandyEquipment;
		boost::signals2::connection m_TemperatureSlot;
		std::optional<std::reference_wrapper<Connection>> m_Connection;
	};


}
// namespace AqualinkAutomate::HTTP
