#include "http/webroute_jandyequipment_version.h"
#include "http/json/json_jandy_equipment.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_JandyEquipment_Version::WebRoute_JandyEquipment_Version(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment) :
		Interfaces::IWebRoute<EQUIPMENTVERSION_ROUTE_URL>(app),
		m_JandyEquipment(jandy_equipment)
	{
	}
	
	void WebRoute_JandyEquipment_Version::WebRequestHandler(const Request& req, Response& resp)
	{
		resp.set_header("Content-Type", "application/json");
		resp.body = JSON::GenerateJson_JandyEquipment_Version(m_JandyEquipment).dump();
		resp.end();
	}

}
// namespace AqualinkAutomate::HTTP
