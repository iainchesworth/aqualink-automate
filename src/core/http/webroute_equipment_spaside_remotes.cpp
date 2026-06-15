#include <algorithm>
#include <cctype>
#include <format>
#include <optional>
#include <source_location>
#include <string>

#include <nlohmann/json.hpp>

#include "http/webroute_equipment_spaside_remotes.h"
#include "http/server/server_fields.h"
#include "logging/logging.h"
#include "preferences/preferences_service.h"
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

	WebRoute_Equipment_SpasideRemotes::WebRoute_Equipment_SpasideRemotes(Kernel::HubLocator& hub_locator, std::shared_ptr<Preferences::PreferencesService> preferences_service) :
		m_PreferencesService(std::move(preferences_service))
	{
		// TryFind (not Find): a spa-side controller is only present when the Jandy stack is running.
		// In dev-mode/replay there is none, and the route should still construct and report an empty
		// list rather than throw.
		m_Controller = hub_locator.TryFind<Interfaces::ISpasideRemoteController>();
		m_DataHub = hub_locator.TryFind<Kernel::DataHub>();
		m_PreferencesHub = hub_locator.TryFind<Kernel::PreferencesHub>();
	}

	HTTP::Response WebRoute_Equipment_SpasideRemotes::OnRequest(const HTTP::Request& req)
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

	HTTP::Response WebRoute_Equipment_SpasideRemotes::HandleGet(const HTTP::Request& req)
	{
		std::vector<Interfaces::ISpasideRemoteController::RemoteState> remotes;
		if (m_Controller)
		{
			remotes = m_Controller->Remotes();
		}
		// With no controller the empty list is the correct picture (no spa-side stack running).

		auto envelope = RemotesToJson(remotes);

		// The controller's decoded button->function assignments (iAQ "Spa Remotes" / OneTouch
		// "Spa Switch" config), keyed by the controller's own switch numbering (1..3).
		nlohmann::json assignments = nlohmann::json::array();
		if (m_DataHub)
		{
			for (const auto& [key, function] : m_DataHub->SpaSwitchAssignments())
			{
				nlohmann::json entry;
				entry["switch"] = key.first;
				entry["button"] = key.second;
				entry["function"] = function;
				assignments.push_back(std::move(entry));
			}
		}
		envelope["assignments"] = std::move(assignments);

		// User-requested (desired-state) assignments persisted in PreferencesHub, keyed
		// "<switch>:<button>". Surfaced so the UI can show intent even before the controller
		// re-reports it (or, on a read-only system, as an annotation).
		nlohmann::json requested = nlohmann::json::array();
		if (m_PreferencesHub)
		{
			auto parse_uint = [](const std::string& s) -> std::optional<unsigned long>
			{
				if (s.empty() || !std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); }))
				{
					return std::nullopt;
				}
				return std::stoul(s);
			};

			for (const auto& [key, function] : m_PreferencesHub->SpaSwitchButtons.items())
			{
				const auto colon = key.find(':');
				if (colon == std::string::npos || !function.is_string()) { continue; }
				const auto sw = parse_uint(key.substr(0, colon));
				const auto btn = parse_uint(key.substr(colon + 1));
				if (!sw.has_value() || !btn.has_value()) { continue; }
				nlohmann::json entry;
				entry["switch"] = sw.value();
				entry["button"] = btn.value();
				entry["function"] = function.get<std::string>();
				requested.push_back(std::move(entry));
			}
		}
		envelope["requested"] = std::move(requested);

		return MakeJsonResponse(req, HTTP::Status::ok, envelope.dump());
	}

	HTTP::Response WebRoute_Equipment_SpasideRemotes::HandlePost(const HTTP::Request& req)
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
				return MakeJsonResponse(req, HTTP::Status::bad_request, R"json({"error":"Request must contain a string 'action' ('press' or 'assign')"})json");
			}

			const auto action = body["action"].get<std::string>();

			if ("press" == action)
			{
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

			if ("assign" == action)
			{
				// Program switch:button -> function over the bus (drives the controller's config menu).
				if (!body.contains("switch") || !body["switch"].is_number_unsigned() ||
					!body.contains("button") || !body["button"].is_number_unsigned() ||
					!body.contains("function") || !body["function"].is_string())
				{
					return MakeJsonResponse(req, HTTP::Status::bad_request, R"({"error":"'assign' requires unsigned 'switch', unsigned 'button', and string 'function'"})");
				}

				const auto switch_value = body["switch"].get<std::uint64_t>();
				const auto button_value = body["button"].get<std::uint64_t>();
				const auto function = body["function"].get<std::string>();
				if ((switch_value > 0xFF) || (button_value > 0xFF))
				{
					return MakeJsonResponse(req, HTTP::Status::bad_request, R"({"error":"'switch' and 'button' must each fit in a single byte"})");
				}

				const auto sw = static_cast<uint8_t>(switch_value);
				const auto btn = static_cast<uint8_t>(button_value);

				switch (m_Controller->SetButtonAssignment(sw, btn, function))
				{
				case Interfaces::ISpasideRemoteController::AssignResult::Accepted:
					LogInfo(Channel::Web, std::format("Spa-switch assign: switch {} button {} -> '{}' queued via web UI", sw, btn, function));
					// Remember the user's request (desired state) so the UI reflects it and it
					// survives a restart; the controller's live decoded map remains authoritative.
					if (m_PreferencesService)
					{
						m_PreferencesService->RecordSpaSwitchAssignment(sw, btn, function);
					}
					return MakeJsonResponse(req, HTTP::Status::ok, RemotesToJson(m_Controller->Remotes()).dump());

				case Interfaces::ISpasideRemoteController::AssignResult::InvalidRequest:
					return MakeJsonResponse(req, HTTP::Status::bad_request, R"({"error":"Invalid switch/button/function for assignment"})");

				case Interfaces::ISpasideRemoteController::AssignResult::Busy:
					return MakeJsonResponse(req, HTTP::Status::conflict, R"({"error":"A controller operation is in progress; retry shortly"})");

				case Interfaces::ISpasideRemoteController::AssignResult::NotAvailable:
				default:
					return MakeJsonResponse(req, HTTP::Status::service_unavailable, R"({"error":"No controller can program spa-switch assignments on this system"})");
				}
			}

			return MakeJsonResponse(req, HTTP::Status::bad_request, R"({"error":"'action' must be 'press' or 'assign'"})");
		}
		catch (const nlohmann::json::exception&)
		{
			return MakeJsonResponse(req, HTTP::Status::bad_request, R"({"error":"Invalid JSON in request body"})");
		}
	}

}
// namespace AqualinkAutomate::HTTP
