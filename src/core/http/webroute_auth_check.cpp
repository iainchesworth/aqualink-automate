#include <source_location>

#include <nlohmann/json.hpp>

#include "http/webroute_auth_check.h"
#include "http/server/server_fields.h"
#include "profiling/factories/profiling_unit_factory.h"

namespace AqualinkAutomate::HTTP
{

	HTTP::Message WebRoute_AuthCheck::OnRequest(const HTTP::Request& req)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WebRoute_AuthCheck::OnRequest", std::source_location::current());

		// Reached only when authorised (or auth disabled) -> always 200.
		nlohmann::json body;
		body["authenticated"] = true;

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
