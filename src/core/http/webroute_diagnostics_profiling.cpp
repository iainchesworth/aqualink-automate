#include <format>
#include <source_location>
#include <string>

#include <nlohmann/json.hpp>

#include "http/webroute_diagnostics_profiling.h"
#include "http/server/server_fields.h"
#include "logging/logging.h"
#include "profiling/factories/profiling_unit_factory.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	namespace
	{
		// Build the status envelope shared by GET and POST responses:
		//   { "enabled": bool, "running": bool, "backend": string, "available": [string] }
		nlohmann::json StatusToJson(const Interfaces::IProfilingController::Status& status)
		{
			nlohmann::json result;
			result["enabled"] = status.enabled;
			result["running"] = status.running;
			result["backend"] = status.active_backend;
			result["available"] = status.available_backends;
			return result;
		}

		HTTP::Response MakeJsonResponse(const HTTP::Request& req, HTTP::Status code, const std::string& body)
		{
			HTTP::Response resp{ code, req.version() };
			resp.set(boost::beast::http::field::server, ServerFields::Server());
			resp.set(boost::beast::http::field::content_type, ContentTypes::APPLICATION_JSON);
			resp.keep_alive(req.keep_alive());
			resp.body() = body;
			resp.prepare_payload();
			return resp;
		}
	}

	WebRoute_Diagnostics_Profiling::WebRoute_Diagnostics_Profiling(Kernel::HubLocator& hub_locator)
	{
		// TryFind (not Find): keep the route resilient even if the controller was
		// not registered — it then reports enabled=false and rejects toggles.
		m_ProfilingController = hub_locator.TryFind<Interfaces::IProfilingController>();
	}

	HTTP::Response WebRoute_Diagnostics_Profiling::OnRequest(const HTTP::Request& req)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WebRoute_Diagnostics_Profiling::OnRequest", std::source_location::current());

		switch (req.method())
		{
		case Verbs::get:
			return HandleGet(req);

		case Verbs::post:
			return HandlePost(req);

		default:
			return MakeJsonResponse(req, HTTP::Status::method_not_allowed, R"({"error":"Method not allowed. Use GET or POST."})");
		}
	}

	HTTP::Response WebRoute_Diagnostics_Profiling::HandleGet(const HTTP::Request& req)
	{
		Interfaces::IProfilingController::Status status;
		if (m_ProfilingController)
		{
			status = m_ProfilingController->ProfilingStatus();
		}
		// With no controller the default-constructed status reports
		// enabled=false / running=false / no backends, which is the correct picture.

		return MakeJsonResponse(req, HTTP::Status::ok, StatusToJson(status).dump());
	}

	HTTP::Response WebRoute_Diagnostics_Profiling::HandlePost(const HTTP::Request& req)
	{
		if (!m_ProfilingController)
		{
			LogWarning(Channel::Web, "Profiling control requested but no profiling controller is available");
			return MakeJsonResponse(req, HTTP::Status::service_unavailable, R"({"error":"Profiling control is not available"})");
		}

		try
		{
			auto body = nlohmann::json::parse(req.body());

			if (!body.contains("action") || !body["action"].is_string())
			{
				return MakeJsonResponse(req, HTTP::Status::bad_request, R"({"error":"Request must contain a string 'action' of 'start', 'stop' or 'select'"})");
			}

			const auto action = body["action"].get<std::string>();

			if ("start" == action)
			{
				if (!m_ProfilingController->Start())
				{
					return MakeJsonResponse(req, HTTP::Status::conflict, R"json({"error":"No profiling backend is available in this build (ENABLE_PROFILING=OFF or backend not selected)"})json");
				}
				LogInfo(Channel::Web, "Profiling capture resumed via web UI");
			}
			else if ("stop" == action)
			{
				if (!m_ProfilingController->Stop())
				{
					return MakeJsonResponse(req, HTTP::Status::conflict, R"json({"error":"No profiling backend is available in this build (ENABLE_PROFILING=OFF or backend not selected)"})json");
				}
				LogInfo(Channel::Web, "Profiling capture paused via web UI");
			}
			else if ("select" == action)
			{
				if (!body.contains("backend") || !body["backend"].is_string())
				{
					return MakeJsonResponse(req, HTTP::Status::bad_request, R"json({"error":"'select' action requires a string 'backend' (tracy, uprof or vtune)"})json");
				}

				const auto backend = body["backend"].get<std::string>();
				if (!m_ProfilingController->SelectBackend(backend))
				{
					return MakeJsonResponse(req, HTTP::Status::conflict, R"({"error":"Requested backend is unknown or was not compiled into this build"})");
				}
				LogInfo(Channel::Web, std::format("Profiling backend '{}' selected via web UI", backend));
			}
			else
			{
				return MakeJsonResponse(req, HTTP::Status::bad_request, R"({"error":"'action' must be 'start', 'stop' or 'select'"})");
			}

			// Success: return the up-to-date status.
			return MakeJsonResponse(req, HTTP::Status::ok, StatusToJson(m_ProfilingController->ProfilingStatus()).dump());
		}
		catch (const nlohmann::json::exception&)
		{
			return MakeJsonResponse(req, HTTP::Status::bad_request, R"({"error":"Invalid JSON in request body"})");
		}
	}

}
// namespace AqualinkAutomate::HTTP
