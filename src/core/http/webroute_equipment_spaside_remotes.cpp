#include <format>
#include <source_location>
#include <string>

#include <nlohmann/json.hpp>

#include "http/webroute_equipment_spaside_remotes.h"
#include "http/server/server_fields.h"
#include "logging/logging.h"
#include "profiling/factories/profiling_unit_factory.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	namespace
	{
		nlohmann::json RemoteToJson(const Interfaces::ISpasideRemoteController::RemoteState& remote)
		{
			nlohmann::json j;
			j["address"] = std::format("0x{:02x}", remote.address);
			j["device_class"] = remote.device_class;
			j["emulated"] = remote.emulated;
			j["button_count"] = remote.button_count;
			j["poll_count"] = remote.poll_count;
			j["last_button"] = remote.last_button;
			j["led_image_seen"] = remote.led_image_seen;
			j["leds"] = remote.leds;
			j["led_image"] = remote.led_image;
			return j;
		}

		nlohmann::json RemotesToJson(const std::vector<Interfaces::ISpasideRemoteController::RemoteState>& remotes)
		{
			nlohmann::json list = nlohmann::json::array();
			for (const auto& remote : remotes)
			{
				list.push_back(RemoteToJson(remote));
			}

			nlohmann::json envelope;
			envelope["remotes"] = std::move(list);
			return envelope;
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

	WebRoute_Equipment_SpasideRemotes::WebRoute_Equipment_SpasideRemotes(Kernel::HubLocator& hub_locator)
	{
		// TryFind (not Find): a spa-side controller is only present when the Jandy stack is running.
		// In dev-mode/replay there is none, and the route should still construct and report an empty
		// list rather than throw.
		m_Controller = hub_locator.TryFind<Interfaces::ISpasideRemoteController>();
	}

	HTTP::Message WebRoute_Equipment_SpasideRemotes::OnRequest(const HTTP::Request& req)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WebRoute_Equipment_SpasideRemotes::OnRequest", std::source_location::current());

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

	HTTP::Message WebRoute_Equipment_SpasideRemotes::HandleGet(const HTTP::Request& req)
	{
		std::vector<Interfaces::ISpasideRemoteController::RemoteState> remotes;
		if (m_Controller)
		{
			remotes = m_Controller->Remotes();
		}
		// With no controller the empty list is the correct picture (no spa-side stack running).

		return MakeJsonResponse(req, HTTP::Status::ok, RemotesToJson(remotes).dump());
	}

	HTTP::Message WebRoute_Equipment_SpasideRemotes::HandlePost(const HTTP::Request& req)
	{
		if (!m_Controller)
		{
			LogWarning(Channel::Web, "Spa-side remote command requested but no controller is available (dev-mode/replay?)");
			return MakeJsonResponse(req, HTTP::Status::service_unavailable, R"({"error":"Spa-side remote control is not available in this mode"})");
		}

		try
		{
			auto body = nlohmann::json::parse(req.body());

			if (!body.contains("action") || !body["action"].is_string())
			{
				return MakeJsonResponse(req, HTTP::Status::bad_request, R"json({"error":"Request must contain a string 'action' (currently only 'press')"})json");
			}

			const auto action = body["action"].get<std::string>();
			if ("press" != action)
			{
				return MakeJsonResponse(req, HTTP::Status::bad_request, R"({"error":"'action' must be 'press'"})");
			}

			if (!body.contains("address") || !body["address"].is_number_unsigned())
			{
				return MakeJsonResponse(req, HTTP::Status::bad_request, R"({"error":"'press' requires an unsigned integer 'address'"})");
			}
			if (!body.contains("button") || !body["button"].is_number_unsigned())
			{
				return MakeJsonResponse(req, HTTP::Status::bad_request, R"({"error":"'press' requires an unsigned integer 'button'"})");
			}

			const auto address_value = body["address"].get<std::uint64_t>();
			const auto button_value = body["button"].get<std::uint64_t>();
			if ((address_value > 0xFF) || (button_value > 0xFF))
			{
				return MakeJsonResponse(req, HTTP::Status::bad_request, R"({"error":"'address' and 'button' must each fit in a single byte"})");
			}

			const auto address = static_cast<uint8_t>(address_value);
			const auto button = static_cast<uint8_t>(button_value);

			switch (m_Controller->PressButton(address, button))
			{
			case Interfaces::ISpasideRemoteController::PressResult::Success:
				LogInfo(Channel::Web, std::format("Spa-side remote 0x{:02x}: queued press of button {} via web UI", address, button));
				return MakeJsonResponse(req, HTTP::Status::ok, RemotesToJson(m_Controller->Remotes()).dump());

			case Interfaces::ISpasideRemoteController::PressResult::RemoteNotFound:
				return MakeJsonResponse(req, HTTP::Status::not_found, R"({"error":"No spa-side remote at that address"})");

			case Interfaces::ISpasideRemoteController::PressResult::NotEmulated:
				return MakeJsonResponse(req, HTTP::Status::conflict, R"({"error":"That spa-side remote is a real device we only observe; it cannot be actuated"})");

			case Interfaces::ISpasideRemoteController::PressResult::InvalidButton:
			default:
				return MakeJsonResponse(req, HTTP::Status::bad_request, R"json({"error":"'button' is out of range for that remote (1..button_count)"})json");
			}
		}
		catch (const nlohmann::json::exception&)
		{
			return MakeJsonResponse(req, HTTP::Status::bad_request, R"({"error":"Invalid JSON in request body"})");
		}
	}

}
// namespace AqualinkAutomate::HTTP
