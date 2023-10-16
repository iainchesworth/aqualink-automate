#include <nlohmann/json.hpp>

#include "formatters/temperature_formatter.h"
#include "http/json/json_equipment.h"
#include "http/server/server_fields.h"
#include "http/webroute_equipment.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Equipment::WebRoute_Equipment(Kernel::HubLocator& hub_locator) : 
		Interfaces::IWebRoute<EQUIPMENT_ROUTE_URL>()
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();
		m_StatisticsHub = hub_locator.Find<Kernel::StatisticsHub>();
	}

	Message WebRoute_Equipment::OnRequest(HTTP::Request req)
	{
		nlohmann::json jandy_equipment_json;

		jandy_equipment_json["spaHeatStatus"] = "On";
		jandy_equipment_json["poolHeatStatus"] = "Enabled";
		jandy_equipment_json["userStatus"] = "Off";
		jandy_equipment_json["cleanStatus"] = "On";

		jandy_equipment_json["temperatures"]["pool"] = std::format("{}", m_DataHub->PoolTemp());
		jandy_equipment_json["temperatures"]["spa"] = std::format("{}", m_DataHub->SpaTemp());
		jandy_equipment_json["temperatures"]["air"] = std::format("{}", m_DataHub->AirTemp());

		jandy_equipment_json["chemistry"]["ph"] = "";
		jandy_equipment_json["chemisty"]["orp"] = "";
		jandy_equipment_json["chemisty"]["salt_in_ppm"] = "";

		jandy_equipment_json["buttons"] = JSON::GenerateJson_Equipment_Buttons(m_DataHub);
		jandy_equipment_json["devices"] = JSON::GenerateJson_Equipment_Devices(m_DataHub);
		jandy_equipment_json["stats"] = JSON::GenerateJson_Equipment_Stats(m_StatisticsHub);
		jandy_equipment_json["version"] = JSON::GenerateJson_Equipment_Version(m_DataHub);

		HTTP::Response resp{HTTP::Status::ok, req.version()};

        resp.set(boost::beast::http::field::server, ServerFields::Server());
        resp.set(boost::beast::http::field::content_type, ContentTypes::APPLICATION_JSON);
        resp.keep_alive(req.keep_alive());
        resp.body() = jandy_equipment_json.dump();
        resp.prepare_payload();

        return resp;
	}

}
// namespace AqualinkAutomate::HTTP
