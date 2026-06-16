#include <chrono>
#include <source_location>

#include <nlohmann/json.hpp>

#include "http/webroute_health.h"
#include "http/server/server_fields.h"
#include "profiling/factories/profiling_unit_factory.h"

namespace AqualinkAutomate::HTTP
{
	namespace
	{
		// Captured at static-init (process start). steady_clock is monotonic, so the
		// reported uptime is immune to wall-clock adjustments (NTP steps, DST).
		const auto SERVER_START_TIME = std::chrono::steady_clock::now();
	}

	HTTP::Response WebRoute_Health::OnRequest(const HTTP::Request& req)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WebRoute_Health::OnRequest", std::source_location::current());

		// Reaching this handler means the HTTP server and the cooperative poll loop
		// are alive and responsive -> the process is healthy.
		const auto uptime_secs = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - SERVER_START_TIME).count();

		nlohmann::json body;
		body["status"] = "ok";
		body["uptime_seconds"] = uptime_secs;

		HTTP::Response resp{ HTTP::Status::ok, req.version() };
		resp.set(boost::beast::http::field::server, ServerFields::Server());
		resp.set(boost::beast::http::field::content_type, ContentTypes::APPLICATION_JSON);
		resp.keep_alive(req.keep_alive());
		resp.body() = body.dump();
		resp.prepare_payload();

		zone->Value(resp.body().size());

		return resp;
	}

}
// namespace AqualinkAutomate::HTTP
