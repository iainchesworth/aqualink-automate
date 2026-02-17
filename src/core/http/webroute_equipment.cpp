#include <nlohmann/json.hpp>

#include "formatters/temperature_formatter.h"
#include "http/json/json_equipment.h"
#include "http/server/server_fields.h"
#include "http/webroute_equipment.h"
#include "profiling/factories/profiling_unit_factory.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Equipment::WebRoute_Equipment(Kernel::HubLocator& hub_locator) : 
		Interfaces::IWebRoute<EQUIPMENT_ROUTE_URL>()
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();
		m_StatisticsHub = hub_locator.Find<Kernel::StatisticsHub>();
	}

	Message WebRoute_Equipment::OnRequest(const HTTP::Request& req)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WebRoute_Equipment::OnRequest", std::source_location::current());

		nlohmann::json jandy_equipment_json;

		jandy_equipment_json["spaHeatStatus"] = "On";
		jandy_equipment_json["poolHeatStatus"] = "Enabled";
		jandy_equipment_json["userStatus"] = "Off";
		jandy_equipment_json["cleanStatus"] = "On";

		jandy_equipment_json["temperatures"]["pool"] = std::format("{}", m_DataHub->PoolTemp());
		jandy_equipment_json["temperatures"]["spa"] = std::format("{}", m_DataHub->SpaTemp());
		jandy_equipment_json["temperatures"]["air"] = std::format("{}", m_DataHub->AirTemp());
		jandy_equipment_json["temperatures"]["pool_setpoint"] = {
			{"celsius", m_DataHub->PoolTempSetpoint().InCelsius().value()},
			{"fahrenheit", m_DataHub->PoolTempSetpoint().InFahrenheit().value()}
		};
		jandy_equipment_json["temperatures"]["spa_setpoint"] = {
			{"celsius", m_DataHub->SpaTempSetpoint().InCelsius().value()},
			{"fahrenheit", m_DataHub->SpaTempSetpoint().InFahrenheit().value()}
		};

		jandy_equipment_json["chemistry"]["ph"] = static_cast<double>(m_DataHub->pH()());
		jandy_equipment_json["chemistry"]["orp"] = static_cast<uint16_t>(m_DataHub->ORP()().value());
		jandy_equipment_json["chemistry"]["salt_in_ppm"] = static_cast<uint16_t>(m_DataHub->SaltLevel().value());

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

        zone->Value(resp.body().size());

        return resp;
	}

}
// namespace AqualinkAutomate::HTTP
