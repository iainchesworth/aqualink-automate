#include <format>
#include <source_location>
#include <string>

#include <nlohmann/json.hpp>

#include "http/webroute_diagnostics_recording.h"
#include "http/server/server_fields.h"
#include "logging/logging.h"
#include "profiling/factories/profiling_unit_factory.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	namespace
	{
		// Build the status envelope shared by GET and POST responses:
		//   { "recording": bool, "file": string, "bytes": number }
		nlohmann::json StatusToJson(const Interfaces::IRecordingController::Status& status)
		{
			nlohmann::json result;
			result["recording"] = status.recording;
			result["file"] = status.file;
			result["bytes"] = status.bytes_written;
			return result;
		}

		HTTP::Message MakeJsonResponse(const HTTP::Request& req, HTTP::Status code, const std::string& body)
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

	WebRoute_Diagnostics_Recording::WebRoute_Diagnostics_Recording(Kernel::HubLocator& hub_locator)
	{
		// TryFind (not Find): the recording controller is only present in the
		// production serial chain. In dev-mode/replay there is none, and the route
		// should still construct and report recording=false rather than throw.
		m_RecordingController = hub_locator.TryFind<Interfaces::IRecordingController>();
	}

	boost::beast::http::message_generator WebRoute_Diagnostics_Recording::OnRequest(const HTTP::Request& req)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WebRoute_Diagnostics_Recording::OnRequest", std::source_location::current());

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

	boost::beast::http::message_generator WebRoute_Diagnostics_Recording::HandleGet(const HTTP::Request& req)
	{
		Interfaces::IRecordingController::Status status;
		if (m_RecordingController)
		{
			status = m_RecordingController->RecordingStatus();
		}
		// When no controller is registered the default-constructed status reports
		// recording=false / empty file / 0 bytes, which is the correct picture.

		return MakeJsonResponse(req, HTTP::Status::ok, StatusToJson(status).dump());
	}

	boost::beast::http::message_generator WebRoute_Diagnostics_Recording::HandlePost(const HTTP::Request& req)
	{
		if (!m_RecordingController)
		{
			LogWarning(Channel::Web, "Recording toggle requested but no recording controller is available (dev-mode/replay?)");
			return MakeJsonResponse(req, HTTP::Status::service_unavailable, R"({"error":"Recording is not available in this mode"})");
		}

		try
		{
			auto body = nlohmann::json::parse(req.body());

			if (!body.contains("action") || !body["action"].is_string())
			{
				return MakeJsonResponse(req, HTTP::Status::bad_request, R"({"error":"Request must contain a string 'action' of 'start' or 'stop'"})");
			}

			const auto action = body["action"].get<std::string>();

			if ("start" == action)
			{
				if (!body.contains("filename") || !body["filename"].is_string())
				{
					return MakeJsonResponse(req, HTTP::Status::bad_request, R"({"error":"'start' action requires a string 'filename'"})");
				}

				const auto filename = body["filename"].get<std::string>();
				if (filename.empty())
				{
					return MakeJsonResponse(req, HTTP::Status::bad_request, R"({"error":"'filename' must not be empty"})");
				}

				if (!m_RecordingController->StartRecording(filename))
				{
					// Already recording, or the file could not be opened.
					LogWarning(Channel::Web, std::format("Failed to start serial recording to '{}' via web UI", filename));
					return MakeJsonResponse(req, HTTP::Status::conflict, R"json({"error":"Could not start recording: already recording or file could not be opened"})json");
				}

				LogInfo(Channel::Web, std::format("Serial recording started to '{}' via web UI", filename));
			}
			else if ("stop" == action)
			{
				if (!m_RecordingController->StopRecording())
				{
					LogDebug(Channel::Web, "Stop recording requested via web UI but nothing was recording");
					return MakeJsonResponse(req, HTTP::Status::conflict, R"({"error":"Not currently recording"})");
				}

				LogInfo(Channel::Web, "Serial recording stopped via web UI");
			}
			else
			{
				return MakeJsonResponse(req, HTTP::Status::bad_request, R"({"error":"'action' must be 'start' or 'stop'"})");
			}

			// Success: return the up-to-date status.
			return MakeJsonResponse(req, HTTP::Status::ok, StatusToJson(m_RecordingController->RecordingStatus()).dump());
		}
		catch (const nlohmann::json::exception&)
		{
			return MakeJsonResponse(req, HTTP::Status::bad_request, R"({"error":"Invalid JSON in request body"})");
		}
	}

}
// namespace AqualinkAutomate::HTTP
