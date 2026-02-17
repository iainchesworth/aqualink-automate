#include <cmath>
#include <format>

#include <nlohmann/json.hpp>

#include "http/webroute_equipment_setpoints.h"
#include "http/server/server_fields.h"
#include "http/server/responses/response_405.h"
#include "logging/logging.h"
#include "profiling/factories/profiling_unit_factory.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	WebRoute_Equipment_Setpoints::WebRoute_Equipment_Setpoints(Kernel::HubLocator& hub_locator)
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();
		m_CommandDispatcher = hub_locator.TryFind<Interfaces::ICommandDispatcher>();
	}

	HTTP::Message WebRoute_Equipment_Setpoints::OnRequest(const HTTP::Request& req)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WebRoute_EquipmentSetpoints::OnRequest", std::source_location::current());

		switch (req.method())
		{
		case HTTP::Verbs::get:
			return HandleGet(req);

		case HTTP::Verbs::post:
			return HandlePost(req);

		default:
			return HTTP::Responses::Response_405(req);
		}
	}

	HTTP::Message WebRoute_Equipment_Setpoints::HandleGet(const HTTP::Request& req)
	{
		nlohmann::json body;

		body["pool_setpoint"] = {
			{"celsius", m_DataHub->PoolTempSetpoint().InCelsius().value()},
			{"fahrenheit", m_DataHub->PoolTempSetpoint().InFahrenheit().value()}
		};
		body["spa_setpoint"] = {
			{"celsius", m_DataHub->SpaTempSetpoint().InCelsius().value()},
			{"fahrenheit", m_DataHub->SpaTempSetpoint().InFahrenheit().value()}
		};

		HTTP::Response resp{ HTTP::Status::ok, req.version() };
		resp.set(boost::beast::http::field::server, ServerFields::Server());
		resp.set(boost::beast::http::field::content_type, ContentTypes::APPLICATION_JSON);
		resp.keep_alive(req.keep_alive());
		resp.body() = body.dump();
		resp.prepare_payload();

		return resp;
	}

	HTTP::Message WebRoute_Equipment_Setpoints::HandlePost(const HTTP::Request& req)
	{
		if (!m_CommandDispatcher)
		{
			HTTP::Response resp{ HTTP::Status::service_unavailable, req.version() };
			resp.set(boost::beast::http::field::server, ServerFields::Server());
			resp.set(boost::beast::http::field::content_type, ContentTypes::TEXT_PLAIN);
			resp.keep_alive(req.keep_alive());
			resp.body() = "Command dispatcher not available";
			resp.prepare_payload();
			return resp;
		}

		nlohmann::json payload;
		try
		{
			payload = nlohmann::json::parse(req.body());
		}
		catch (const nlohmann::json::parse_error&)
		{
			HTTP::Response resp{ HTTP::Status::bad_request, req.version() };
			resp.set(boost::beast::http::field::server, ServerFields::Server());
			resp.set(boost::beast::http::field::content_type, ContentTypes::TEXT_PLAIN);
			resp.keep_alive(req.keep_alive());
			resp.body() = "Invalid JSON";
			resp.prepare_payload();
			return resp;
		}

		nlohmann::json result;

		auto convert_and_dispatch = [&](const std::string& key, auto dispatch_fn) -> bool
		{
			if (!payload.contains(key))
			{
				return true; // field not present = OK, skip
			}

			double celsius = payload[key].get<double>();

			uint8_t temp_value = 0;
			if (m_DataHub->SystemTemperatureUnits() == Kernel::TemperatureUnits::Celsius)
			{
				temp_value = static_cast<uint8_t>(std::round(celsius));
			}
			else
			{
				temp_value = static_cast<uint8_t>(std::round(celsius * 9.0 / 5.0 + 32.0));
			}

			auto cmd_result = dispatch_fn(temp_value);
			result[key] = {
				{"status", cmd_result == Interfaces::ICommandDispatcher::CommandResult::Success ? "success" : "error"},
				{"celsius", celsius}
			};

			return cmd_result == Interfaces::ICommandDispatcher::CommandResult::Success;
		};

		convert_and_dispatch("pool", [&](uint8_t temp) { return m_CommandDispatcher->SetPoolSetpoint(temp); });
		convert_and_dispatch("spa", [&](uint8_t temp) { return m_CommandDispatcher->SetSpaSetpoint(temp); });

		HTTP::Response resp{ HTTP::Status::ok, req.version() };
		resp.set(boost::beast::http::field::server, ServerFields::Server());
		resp.set(boost::beast::http::field::content_type, ContentTypes::APPLICATION_JSON);
		resp.keep_alive(req.keep_alive());
		resp.body() = result.dump();
		resp.prepare_payload();

		return resp;
	}

}
// namespace AqualinkAutomate::HTTP
