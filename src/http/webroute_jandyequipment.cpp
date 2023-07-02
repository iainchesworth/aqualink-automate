#include <nlohmann/json.hpp>

#include "http/webroute_jandyequipment.h"
#include "http/json/json_jandy_equipment.h"
#include "jandy/formatters/temperature_formatter.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_JandyEquipment::WebRoute_JandyEquipment(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment) :
		Interfaces::IWebRoute<EQUIPMENT_ROUTE_URL>(app, {{ crow::HTTPMethod::Get, std::bind(&WebRoute_JandyEquipment::WebGetRequestHandler, this, std::placeholders::_1, std::placeholders::_2) }}),
		m_JandyEquipment(jandy_equipment)
	{
	}

	void WebRoute_JandyEquipment::WebGetRequestHandler(const HTTP::Request& req, HTTP::Response& resp)
	{
		nlohmann::json jandy_equipment_json;

		jandy_equipment_json["spaHeatStatus"] = "On";
		jandy_equipment_json["poolHeatStatus"] = "Enabled";
		jandy_equipment_json["userStatus"] = "Off";
		jandy_equipment_json["cleanStatus"] = "On";

		jandy_equipment_json["temperatures"]["pool"] = std::format("{}", m_JandyEquipment.Config().PoolTemp());
		jandy_equipment_json["temperatures"]["spa"] = std::format("{}", m_JandyEquipment.Config().SpaTemp());
		jandy_equipment_json["temperatures"]["air"] = std::format("{}", m_JandyEquipment.Config().AirTemp());

		jandy_equipment_json["chemistry"]["ph"] = "";
		jandy_equipment_json["chemisty"]["orp"] = "";
		jandy_equipment_json["chemisty"]["salt_in_ppm"] = "";

		jandy_equipment_json["buttons"] = JSON::GenerateJson_JandyEquipment_Buttons(m_JandyEquipment);
		jandy_equipment_json["devices"] = JSON::GenerateJson_JandyEquipment_Devices(m_JandyEquipment);
		jandy_equipment_json["stats"] = JSON::GenerateJson_JandyEquipment_Stats(m_JandyEquipment);
		jandy_equipment_json["version"] = JSON::GenerateJson_JandyEquipment_Version(m_JandyEquipment);

		resp.set_header("Content-Type", "application/json");
		resp.body = jandy_equipment_json.dump();
		resp.end();
	}

}
// namespace AqualinkAutomate::HTTP
