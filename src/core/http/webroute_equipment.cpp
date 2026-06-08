#include <nlohmann/json.hpp>
#include <magic_enum/magic_enum.hpp>

#include "http/json/json_equipment.h"
#include "http/server/make_response.h"
#include "http/webroute_equipment.h"
#include "profiling/factories/profiling_unit_factory.h"
#include "utility/json_serialization_helpers.h"

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

		jandy_equipment_json["temperatures"]["pool"] = Utility::SerializeTemperature(m_DataHub->PoolTemp());
		jandy_equipment_json["temperatures"]["spa"] = Utility::SerializeTemperature(m_DataHub->SpaTemp());
		jandy_equipment_json["temperatures"]["air"] = Utility::SerializeTemperature(m_DataHub->AirTemp());
		jandy_equipment_json["temperatures"]["pool_setpoint"] = Utility::SerializeTemperature(m_DataHub->PoolTempSetpoint());
		jandy_equipment_json["temperatures"]["spa_setpoint"] = Utility::SerializeTemperature(m_DataHub->SpaTempSetpoint());

		jandy_equipment_json["chemistry"]["ph"] = static_cast<double>(m_DataHub->pH()());
		jandy_equipment_json["chemistry"]["orp"] = static_cast<uint16_t>(m_DataHub->ORP()().value());
		jandy_equipment_json["chemistry"]["salt_in_ppm"] = static_cast<uint16_t>(m_DataHub->SaltLevel().value());

		// Configuration section with body-of-water info.
		{
			nlohmann::json config;
			config["pool_configuration"] = std::string{ magic_enum::enum_name(m_DataHub->PoolConfiguration) };
			config["configuration_source"] = std::string{ magic_enum::enum_name(m_DataHub->PoolConfigurationSource) };

			nlohmann::json bodies_array = nlohmann::json::array();
			for (const auto& body : m_DataHub->Bodies())
			{
				nlohmann::json body_json;
				body_json["id"] = std::string{ magic_enum::enum_name(body.Id()) };
				body_json["label"] = body.Label();
				body_json["is_active"] = body.IsActive();
				body_json["temperature"] = Utility::SerializeTemperature(body.CurrentTemp());
				body_json["setpoint"] = Utility::SerializeTemperature(body.TempSetpoint());
				bodies_array.push_back(std::move(body_json));
			}
			config["bodies"] = std::move(bodies_array);

			jandy_equipment_json["configuration"] = std::move(config);
		}

		jandy_equipment_json["devices"] = JSON::GenerateJson_Equipment_Devices(m_DataHub);
		jandy_equipment_json["stats"] = JSON::GenerateJson_Equipment_Stats(m_StatisticsHub);
		jandy_equipment_json["version"] = JSON::GenerateJson_Equipment_Version(m_DataHub);

		auto resp = MakeJsonResponse(req, HTTP::Status::ok, jandy_equipment_json.dump());

		zone->Value(resp.body().size());

		return resp;
	}

}
// namespace AqualinkAutomate::HTTP
