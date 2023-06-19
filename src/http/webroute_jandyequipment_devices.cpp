#include "http/webroute_jandyequipment_devices.h"
#include "http/json/json_jandy_equipment.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_JandyEquipment_Devices::WebRoute_JandyEquipment_Devices(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment) :
		Interfaces::IWebRoute<EQUIPMENTDEVICES_ROUTE_URL>(app),
		m_JandyEquipment(jandy_equipment)
	{
	}

	void WebRoute_JandyEquipment_Devices::WebRequestHandler(const Request& req, Response& resp)
	{
		resp.set_header("Content-Type", "application/json");
		resp.body = JSON::GenerateJson_JandyEquipment_Devices(m_JandyEquipment).dump();
		resp.end();
	}

}
// namespace AqualinkAutomate::HTTP
