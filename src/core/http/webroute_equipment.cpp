#include <cstdint>

#include <nlohmann/json.hpp>
#include <magic_enum/magic_enum.hpp>

#include "http/json/json_equipment.h"
#include "http/server/make_response.h"
#include "http/webroute_equipment.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "profiling/factories/profiling_unit_factory.h"
#include "utility/json_serialization_helpers.h"

namespace AqualinkAutomate::HTTP
{

	namespace
	{
		// Build the chlorinator sub-object of the chemistry payload from the
		// DataHub chlorinator AuxillaryDevice traits.  Returns a JSON null when
		// no chlorinator (SWG) has been discovered so the UI can render the SWG
		// fields as placeholders rather than fabricating zeroes.
		nlohmann::json BuildChlorinatorJson(const std::shared_ptr<Kernel::DataHub>& data_hub)
		{
			using namespace Kernel::AuxillaryTraitsTypes;

			auto chlorinators = data_hub->Chlorinators();
			if (chlorinators.empty())
			{
				return nlohmann::json{};
			}

			const auto& device = chlorinators.front();
			nlohmann::json chlorinator;

			chlorinator["generating_percent"] = device->AuxillaryTraits.Has(GeneratingPercentageTrait{})
				? nlohmann::json(static_cast<uint8_t>(*(device->AuxillaryTraits[GeneratingPercentageTrait{}])))
				: nlohmann::json{};

			chlorinator["duty_cycle"] = device->AuxillaryTraits.Has(DutyCycleTrait{})
				? nlohmann::json(static_cast<uint8_t>(*(device->AuxillaryTraits[DutyCycleTrait{}])))
				: nlohmann::json{};

			chlorinator["status"] = device->AuxillaryTraits.Has(ChlorinatorStatusTrait{})
				? nlohmann::json(std::string{ magic_enum::enum_name(*(device->AuxillaryTraits[ChlorinatorStatusTrait{}])) })
				: nlohmann::json(std::string{ magic_enum::enum_name(Kernel::ChlorinatorStatuses::Unknown) });

			chlorinator["health"] = device->AuxillaryTraits.Has(ChlorinatorHealthTrait{})
				? nlohmann::json(std::string{ magic_enum::enum_name(*(device->AuxillaryTraits[ChlorinatorHealthTrait{}])) })
				: nlohmann::json(std::string{ magic_enum::enum_name(Kernel::ChlorinatorHealth::Unknown) });

			return chlorinator;
		}
	}
	// namespace

	WebRoute_Equipment::WebRoute_Equipment(Kernel::HubLocator& hub_locator) : 
		Interfaces::IWebRoute<EQUIPMENT_ROUTE_URL>()
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();
		m_StatisticsHub = hub_locator.Find<Kernel::StatisticsHub>();
		m_PreferencesHub = hub_locator.Find<Kernel::PreferencesHub>();
	}

	HTTP::Response WebRoute_Equipment::OnRequest(const HTTP::Request& req)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WebRoute_Equipment::OnRequest", std::source_location::current());

		nlohmann::json jandy_equipment_json;

		jandy_equipment_json["temperatures"]["pool"] = Utility::SerializeTemperature(m_DataHub->PoolTemp());
		jandy_equipment_json["temperatures"]["spa"] = Utility::SerializeTemperature(m_DataHub->SpaTemp());
		jandy_equipment_json["temperatures"]["air"] = Utility::SerializeTemperature(m_DataHub->AirTemp());
		jandy_equipment_json["temperatures"]["pool_setpoint"] = Utility::SerializeTemperature(m_DataHub->PoolTempSetpoint());
		jandy_equipment_json["temperatures"]["spa_setpoint"] = Utility::SerializeTemperature(m_DataHub->SpaTempSetpoint());

		// Nested chemistry payload.  Salt / SWG values come from the DataHub
		// chlorinator AuxillaryDevice traits (+ SaltLevel); ORP / pH read the
		// DataHub chemistry values, which are placeholders today (no sensor on
		// the wire yet) but render automatically once a sensor is decoded.  A
		// reported value of exactly 0 means "no sensor" and is emitted as null
		// so the UI shows a "--" placeholder instead of a misleading zero.
		{
			const auto orp_mv = static_cast<uint16_t>(m_DataHub->ORP()().value());
			const auto ph_value = static_cast<double>(m_DataHub->pH()());
			const auto salt_ppm = static_cast<uint16_t>(m_DataHub->SaltLevel().value());

			nlohmann::json chemistry;
			chemistry["salt_ppm"] = salt_ppm;
			chemistry["orp_mv"] = (0 == orp_mv) ? nlohmann::json{} : nlohmann::json(orp_mv);
			chemistry["ph"] = (0.0 == ph_value) ? nlohmann::json{} : nlohmann::json(ph_value);
			chemistry["chlorinator"] = BuildChlorinatorJson(m_DataHub);

			jandy_equipment_json["chemistry"] = std::move(chemistry);
		}

		// Configuration section with body-of-water info.
		{
			nlohmann::json config;
			config["pool_configuration"] = std::string{ magic_enum::enum_name(m_DataHub->PoolConfiguration) };
			config["configuration_source"] = std::string{ magic_enum::enum_name(m_DataHub->PoolConfigurationSource) };
			config["expected_auxillary_count"] = static_cast<int>(m_DataHub->ExpectedAuxillaryCount);
			config["expected_power_center_count"] = static_cast<int>(m_DataHub->ExpectedPowerCenterCount);

			// Equipment validation (discovered set vs the model's expected layout). Null until
			// the startup scrape completes; thereafter exposes counts + any anomalies so a
			// short/incomplete scrape or a mis-wired panel is visible to the UI/integrations.
			if (m_DataHub->EquipmentValidationResult.has_value())
			{
				const auto& validation_result = m_DataHub->EquipmentValidationResult.value();
				nlohmann::json validation;
				validation["passed"] = validation_result.Passed();
				validation["expected_auxillaries"] = static_cast<int>(validation_result.ExpectedAuxillaries);
				validation["discovered_auxillaries"] = static_cast<int>(validation_result.DiscoveredAuxillaries);
				validation["expected_power_centers"] = static_cast<int>(validation_result.ExpectedPowerCenters);
				validation["discovered_power_centers"] = static_cast<int>(validation_result.DiscoveredPowerCenters);
				validation["anomalies"] = validation_result.Anomalies;
				config["validation"] = std::move(validation);
			}
			else
			{
				config["validation"] = nlohmann::json{};
			}

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

		jandy_equipment_json["devices"] = JSON::GenerateJson_Equipment_Devices(m_DataHub, m_PreferencesHub ? m_PreferencesHub->LabelOverrides : nlohmann::json::object());
		jandy_equipment_json["stats"] = JSON::GenerateJson_Equipment_Stats(m_StatisticsHub);
		jandy_equipment_json["version"] = JSON::GenerateJson_Equipment_Version(m_DataHub);

		auto resp = MakeJsonResponse(req, HTTP::Status::ok, jandy_equipment_json.dump());

		zone->Value(resp.body().size());

		return resp;
	}

}
// namespace AqualinkAutomate::HTTP
