#include "http/webroute_jandyequipment_buttons.h"
#include "http/json/json_jandy_equipment.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_JandyEquipment_Buttons::WebRoute_JandyEquipment_Buttons(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment) :
		Interfaces::IWebRoute<EQUIPMENTBUTTONS_ROUTE_URL>(app),
		m_JandyEquipment(jandy_equipment)
	{
	}

	void WebRoute_JandyEquipment_Buttons::WebRequestHandler(const Request& req, Response& resp)
	{
		resp.set_header("Content-Type", "application/json");
		resp.body = JSON::GenerateJson_JandyEquipment_Buttons(m_JandyEquipment).dump();
		resp.end();
	}

}
// namespace AqualinkAutomate::HTTP
