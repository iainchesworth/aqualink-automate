#include "http/websocket_jandyequipment.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebSocket_JandyEquipment::WebSocket_JandyEquipment(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment) :
		Interfaces::IWebSocket<EQUIPMENT_WEBSOCKET_URL>(app),
		m_JandyEquipment(jandy_equipment)
	{
	}

	void WebSocket_JandyEquipment::OnOpen(Connection& conn)
	{
	}

	void WebSocket_JandyEquipment::OnMessage(Connection& conn, const std::string& data, bool is_binary)
	{
	}

	void WebSocket_JandyEquipment::OnClose(Connection& conn, const std::string& reason)
	{
	}

	void WebSocket_JandyEquipment::OnError(Connection& conn)
	{
	}

}
// namespace AqualinkAutomate::HTTP
