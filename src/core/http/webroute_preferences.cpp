#include <source_location>
#include <string>

#include <nlohmann/json.hpp>

#include "http/server/make_response.h"
#include "http/server/responses/response_405.h"
#include "http/server/responses/response_503.h"
#include "http/server/server_fields.h"
#include "http/server/server_types.h"
#include "http/webroute_preferences.h"
#include "preferences/preferences_service.h"
#include "profiling/factories/profiling_unit_factory.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_Preferences::WebRoute_Preferences(std::shared_ptr<Preferences::PreferencesService> service) :
		m_Service(std::move(service))
	{
	}

	HTTP::Response WebRoute_Preferences::OnRequest(const HTTP::Request& req)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WebRoute_Preferences::OnRequest", std::source_location::current());

		if (!m_Service)
		{
			return HTTP::Responses::Response_503(req);
		}

		switch (req.method())
		{
		case HTTP::Verbs::get:
			return MakeJsonResponse(req, HTTP::Status::ok, m_Service->ToJson().dump());

		case HTTP::Verbs::put:
		{
			auto json = nlohmann::json::parse(req.body(), nullptr, /*allow_exceptions=*/false);
			if (json.is_discarded())
			{
				return MakeResponse(req, HTTP::Status::bad_request, ContentTypes::TEXT_PLAIN, "request body is not valid JSON");
			}

			std::string error;
			if (!m_Service->ApplyJson(json, error))
			{
				return MakeResponse(req, HTTP::Status::bad_request, ContentTypes::TEXT_PLAIN, error);
			}
			return MakeJsonResponse(req, HTTP::Status::ok, m_Service->ToJson().dump());
		}

		default:
			return HTTP::Responses::Response_405(req);
		}
	}

}
// namespace AqualinkAutomate::HTTP
