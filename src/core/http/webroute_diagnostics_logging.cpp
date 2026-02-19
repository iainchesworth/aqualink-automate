#include <string>

#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_utility.hpp>
#include <nlohmann/json.hpp>

#include "http/webroute_diagnostics_logging.h"
#include "http/server/server_fields.h"
#include "logging/logging.h"
#include "logging/logging_channels.h"
#include "logging/logging_severity_filter.h"
#include "logging/logging_severity_levels.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	boost::beast::http::message_generator WebRoute_Diagnostics_Logging::OnRequest(const HTTP::Request& req)
	{
		switch (req.method())
		{
		case Verbs::get:
			return HandleGet(req);

		case Verbs::post:
			return HandlePost(req);

		default:
			HTTP::Response resp{ HTTP::Status::method_not_allowed, req.version() };
			resp.set(boost::beast::http::field::server, ServerFields::Server());
			resp.set(boost::beast::http::field::content_type, ContentTypes::APPLICATION_JSON);
			resp.keep_alive(req.keep_alive());
			resp.body() = R"({"error":"Method not allowed. Use GET or POST."})";
			resp.prepare_payload();
			return resp;
		}
	}

	boost::beast::http::message_generator WebRoute_Diagnostics_Logging::HandleGet(const HTTP::Request& req)
	{
		nlohmann::json result;

		// Build channel → current severity map
		nlohmann::json channels = nlohmann::json::object();
		magic_enum::enum_for_each<Channel>([&channels](auto const& channel)
			{
				auto severity = SeverityFiltering::GetChannelFilterLevel(channel.value);
				channels[std::string(magic_enum::enum_name(channel.value))] = std::string(magic_enum::enum_name(severity));
			});

		result["channels"] = channels;

		// Build available severity levels array
		nlohmann::json levels = nlohmann::json::array();
		magic_enum::enum_for_each<Severity>([&levels](auto const& severity)
			{
				levels.push_back(std::string(magic_enum::enum_name(severity.value)));
			});

		result["severity_levels"] = levels;

		HTTP::Response resp{ HTTP::Status::ok, req.version() };
		resp.set(boost::beast::http::field::server, ServerFields::Server());
		resp.set(boost::beast::http::field::content_type, ContentTypes::APPLICATION_JSON);
		resp.keep_alive(req.keep_alive());
		resp.body() = result.dump();
		resp.prepare_payload();

		return resp;
	}

	boost::beast::http::message_generator WebRoute_Diagnostics_Logging::HandlePost(const HTTP::Request& req)
	{
		try
		{
			auto body = nlohmann::json::parse(req.body());

			if (body.contains("global"))
			{
				auto level_name = body["global"].get<std::string>();
				auto severity = magic_enum::enum_cast<Severity>(level_name);

				if (!severity.has_value())
				{
					HTTP::Response resp{ HTTP::Status::bad_request, req.version() };
					resp.set(boost::beast::http::field::server, ServerFields::Server());
					resp.set(boost::beast::http::field::content_type, ContentTypes::APPLICATION_JSON);
					resp.keep_alive(req.keep_alive());
					resp.body() = R"({"error":"Invalid severity level"})";
					resp.prepare_payload();
					return resp;
				}

				SeverityFiltering::SetGlobalFilterLevel(severity.value());

				LogInfo(Channel::Web, std::format("Global log level set to {} via web UI", level_name));
			}
			else if (body.contains("channel") && body.contains("level"))
			{
				auto channel_name = body["channel"].get<std::string>();
				auto level_name = body["level"].get<std::string>();

				auto channel = magic_enum::enum_cast<Channel>(channel_name);
				auto severity = magic_enum::enum_cast<Severity>(level_name);

				if (!channel.has_value() || !severity.has_value())
				{
					HTTP::Response resp{ HTTP::Status::bad_request, req.version() };
					resp.set(boost::beast::http::field::server, ServerFields::Server());
					resp.set(boost::beast::http::field::content_type, ContentTypes::APPLICATION_JSON);
					resp.keep_alive(req.keep_alive());
					resp.body() = R"({"error":"Invalid channel or severity level"})";
					resp.prepare_payload();
					return resp;
				}

				SeverityFiltering::SetChannelFilterLevel(channel.value(), severity.value());

				LogInfo(Channel::Web, std::format("Log level for channel {} set to {} via web UI", channel_name, level_name));
			}
			else
			{
				HTTP::Response resp{ HTTP::Status::bad_request, req.version() };
				resp.set(boost::beast::http::field::server, ServerFields::Server());
				resp.set(boost::beast::http::field::content_type, ContentTypes::APPLICATION_JSON);
				resp.keep_alive(req.keep_alive());
				resp.body() = R"({"error":"Request must contain 'global' or both 'channel' and 'level'"})";
				resp.prepare_payload();
				return resp;
			}

			HTTP::Response resp{ HTTP::Status::ok, req.version() };
			resp.set(boost::beast::http::field::server, ServerFields::Server());
			resp.set(boost::beast::http::field::content_type, ContentTypes::APPLICATION_JSON);
			resp.keep_alive(req.keep_alive());
			resp.body() = R"({"status":"ok"})";
			resp.prepare_payload();

			return resp;
		}
		catch (const nlohmann::json::exception&)
		{
			HTTP::Response resp{ HTTP::Status::bad_request, req.version() };
			resp.set(boost::beast::http::field::server, ServerFields::Server());
			resp.set(boost::beast::http::field::content_type, ContentTypes::APPLICATION_JSON);
			resp.keep_alive(req.keep_alive());
			resp.body() = R"({"error":"Invalid JSON in request body"})";
			resp.prepare_payload();

			return resp;
		}
	}

}
// namespace AqualinkAutomate::HTTP
