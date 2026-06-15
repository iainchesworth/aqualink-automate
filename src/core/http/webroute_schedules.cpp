#include <optional>
#include <source_location>
#include <string>

#include <boost/url/parse.hpp>
#include <nlohmann/json.hpp>

#include "http/server/make_response.h"
#include "http/server/responses/response_405.h"
#include "http/server/responses/response_503.h"
#include "http/server/server_fields.h"
#include "http/server/server_types.h"
#include "http/webroute_schedules.h"
#include "profiling/factories/profiling_unit_factory.h"
#include "scheduling/scheduler_service.h"

namespace AqualinkAutomate::HTTP
{
	namespace
	{
		// Last path segment, used to pull {uuid} out of /api/schedules/{uuid}.
		std::optional<std::string> LastPathSegment(const HTTP::Request& req)
		{
			auto url = boost::urls::parse_origin_form(req.target());
			if (!url.has_value()) { return std::nullopt; }
			auto segments = url->segments();
			if (segments.empty()) { return std::nullopt; }
			auto it = segments.end();
			--it;
			std::string last(*it);
			if (last.empty()) { return std::nullopt; }
			return last;
		}

		// Parse + validate a schedule from the request body. Returns nullopt and a
		// 400 response (via out-param) on any error.
		std::optional<Scheduling::Schedule> ParseScheduleBody(const HTTP::Request& req, std::string& error)
		{
			auto json = nlohmann::json::parse(req.body(), nullptr, /*allow_exceptions=*/false);
			if (json.is_discarded())
			{
				error = "request body is not valid JSON";
				return std::nullopt;
			}
			return Scheduling::FromJson(json, error);
		}
	}
	// unnamed namespace

	//-------------------------------------------------------------------------
	// Collection: /api/schedules
	//-------------------------------------------------------------------------
	WebRoute_Schedules::WebRoute_Schedules(std::shared_ptr<Scheduling::SchedulerService> service) :
		m_Service(std::move(service))
	{
	}

	HTTP::Response WebRoute_Schedules::OnRequest(const HTTP::Request& req)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WebRoute_Schedules::OnRequest", std::source_location::current());

		if (!m_Service)
		{
			return HTTP::Responses::Response_503(req);
		}

		switch (req.method())
		{
		case HTTP::Verbs::get:
		{
			nlohmann::json arr = nlohmann::json::array();
			for (const auto& schedule : m_Service->List())
			{
				arr.push_back(Scheduling::ToJson(schedule));
			}
			return MakeJsonResponse(req, HTTP::Status::ok, arr.dump());
		}

		case HTTP::Verbs::post:
		{
			std::string error;
			auto schedule = ParseScheduleBody(req, error);
			if (!schedule.has_value())
			{
				return MakeResponse(req, HTTP::Status::bad_request, ContentTypes::TEXT_PLAIN, error);
			}
			auto created = m_Service->Create(std::move(*schedule));
			return MakeJsonResponse(req, HTTP::Status::created, Scheduling::ToJson(created).dump());
		}

		default:
			return HTTP::Responses::Response_405(req);
		}
	}

	//-------------------------------------------------------------------------
	// Item: /api/schedules/{uuid}
	//-------------------------------------------------------------------------
	WebRoute_Schedule::WebRoute_Schedule(std::shared_ptr<Scheduling::SchedulerService> service) :
		m_Service(std::move(service))
	{
	}

	HTTP::Response WebRoute_Schedule::OnRequest(const HTTP::Request& req)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WebRoute_Schedule::OnRequest", std::source_location::current());

		if (!m_Service)
		{
			return HTTP::Responses::Response_503(req);
		}

		auto uuid = LastPathSegment(req);
		if (!uuid.has_value())
		{
			return MakeResponse(req, HTTP::Status::bad_request, ContentTypes::TEXT_PLAIN, "missing schedule uuid");
		}

		switch (req.method())
		{
		case HTTP::Verbs::get:
		{
			if (auto schedule = m_Service->Get(*uuid); schedule.has_value())
			{
				return MakeJsonResponse(req, HTTP::Status::ok, Scheduling::ToJson(*schedule).dump());
			}
			return MakeResponse(req, HTTP::Status::not_found, ContentTypes::TEXT_PLAIN, "unknown schedule");
		}

		case HTTP::Verbs::put:
		{
			std::string error;
			auto schedule = ParseScheduleBody(req, error);
			if (!schedule.has_value())
			{
				return MakeResponse(req, HTTP::Status::bad_request, ContentTypes::TEXT_PLAIN, error);
			}
			if (!m_Service->Replace(*uuid, std::move(*schedule)))
			{
				return MakeResponse(req, HTTP::Status::not_found, ContentTypes::TEXT_PLAIN, "unknown schedule");
			}
			auto updated = m_Service->Get(*uuid);
			return MakeJsonResponse(req, HTTP::Status::ok, Scheduling::ToJson(*updated).dump());
		}

		case HTTP::Verbs::delete_:
		{
			if (!m_Service->Remove(*uuid))
			{
				return MakeResponse(req, HTTP::Status::not_found, ContentTypes::TEXT_PLAIN, "unknown schedule");
			}
			return MakeResponse(req, HTTP::Status::no_content, ContentTypes::TEXT_PLAIN, "");
		}

		default:
			return HTTP::Responses::Response_405(req);
		}
	}

}
// namespace AqualinkAutomate::HTTP
