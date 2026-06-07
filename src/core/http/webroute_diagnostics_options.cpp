#include <nlohmann/json.hpp>

#include "http/webroute_diagnostics_options.h"
#include "http/server/server_fields.h"
#include "options/options_option_type.h"

namespace AqualinkAutomate::HTTP
{

	boost::beast::http::message_generator WebRoute_Diagnostics_Options::OnRequest(const HTTP::Request& req)
	{
		switch (req.method())
		{
		case Verbs::get:
			return HandleGet(req);

		default:
			HTTP::Response resp{ HTTP::Status::method_not_allowed, req.version() };
			resp.set(boost::beast::http::field::server, ServerFields::Server());
			resp.set(boost::beast::http::field::content_type, ContentTypes::APPLICATION_JSON);
			resp.keep_alive(req.keep_alive());
			resp.body() = R"({"error":"Method not allowed. Use GET."})";
			resp.prepare_payload();
			return resp;
		}
	}

	boost::beast::http::message_generator WebRoute_Diagnostics_Options::HandleGet(const HTTP::Request& req)
	{
		nlohmann::json options = nlohmann::json::array();
		for (const auto& meta : Options::AppOption::RegisteredOptions())
		{
			nlohmann::json entry = nlohmann::json::object();
			entry["name"] = meta.long_name;
			entry["short_name"] = meta.short_name;
			entry["description"] = meta.description;
			options.push_back(entry);
		}

		nlohmann::json result;
		result["options"] = options;

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
