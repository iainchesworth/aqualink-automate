#include <nlohmann/json.hpp>

#include "formatters/temperature_formatter.h"
#include "http/json/json_equipment.h"
#include "http/webroute_equipment.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Equipment::WebRoute_Equipment(HTTP::Server& http_server, const Kernel::DataHub& data_hub, const Kernel::StatisticsHub& statistics_hub) :
		Interfaces::IWebRoute<EQUIPMENT_ROUTE_URL>(http_server, {{ HTTP::Methods::GET, std::bind(&WebRoute_Equipment::WebGetRequestHandler, this, std::placeholders::_1, std::placeholders::_2) }}),
		Interfaces::IShareableRoute(),
		m_DataHub(data_hub),
		m_StatisticsHub(statistics_hub)
	{
	}

	void WebRoute_Equipment::WebGetRequestHandler(const HTTP::Request& req, HTTP::Response& resp)
	{
		nlohmann::json jandy_equipment_json;

		jandy_equipment_json["spaHeatStatus"] = "On";
		jandy_equipment_json["poolHeatStatus"] = "Enabled";
		jandy_equipment_json["userStatus"] = "Off";
		jandy_equipment_json["cleanStatus"] = "On";

		jandy_equipment_json["temperatures"]["pool"] = std::format("{}", m_DataHub.PoolTemp());
		jandy_equipment_json["temperatures"]["spa"] = std::format("{}", m_DataHub.SpaTemp());
		jandy_equipment_json["temperatures"]["air"] = std::format("{}", m_DataHub.AirTemp());

		jandy_equipment_json["chemistry"]["ph"] = "";
		jandy_equipment_json["chemisty"]["orp"] = "";
		jandy_equipment_json["chemisty"]["salt_in_ppm"] = "";

		jandy_equipment_json["buttons"] = JSON::GenerateJson_Equipment_Buttons(m_DataHub);
		jandy_equipment_json["devices"] = JSON::GenerateJson_Equipment_Devices(m_DataHub);
		jandy_equipment_json["stats"] = JSON::GenerateJson_Equipment_Stats(m_StatisticsHub);
		jandy_equipment_json["version"] = JSON::GenerateJson_Equipment_Version(m_DataHub);

		resp.set_status_and_content(
			cinatra::status_type::ok, 
			jandy_equipment_json.dump(),
			cinatra::req_content_type::json, 
			cinatra::content_encoding::none
		);
	}

}
// namespace AqualinkAutomate::HTTP
