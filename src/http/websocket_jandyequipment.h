#pragma once

#include <string>

#include <boost/signals2.hpp>
#include <crow/app.h>

#include "interfaces/iwebsocket.h"
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
		const Equipment::JandyEquipment& m_JandyEquipment;
	};


}
// namespace AqualinkAutomate::HTTP
