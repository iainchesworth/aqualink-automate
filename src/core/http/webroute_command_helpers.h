#pragma once

#include <memory>
#include <optional>

#include <nlohmann/json.hpp>

#include "http/server/make_response.h"
#include "http/server/server_fields.h"
#include "http/server/server_types.h"
#include "interfaces/icommanddispatcher.h"

// Shared helpers for the equipment command web routes (heater, circulation, ...). These collapse
// the boilerplate every command route repeats (result->status mapping, the no-dispatcher guard,
// and JSON-object body parsing) into one place so each route only carries its own field handling.
namespace AqualinkAutomate::HTTP
{
	// Map a command-dispatch result to the HTTP status a route should return.
	inline HTTP::Status StatusForCommandResult(Interfaces::ICommandDispatcher::CommandResult result)
	{
		using CommandResult = Interfaces::ICommandDispatcher::CommandResult;
		switch (result)
		{
		case CommandResult::Success:              return HTTP::Status::ok;
		case CommandResult::InvalidValue:         return HTTP::Status::bad_request;
		case CommandResult::DeviceNotFound:
		case CommandResult::NoSerialAdapter:      return HTTP::Status::service_unavailable;
		case CommandResult::UnknownEquipmentType: return HTTP::Status::unprocessable_entity;
		default:                                  return HTTP::Status::internal_server_error;
		}
	}

	// Returns a 503 response when no command dispatcher is wired, else std::nullopt.
	inline std::optional<HTTP::Response> RequireCommandDispatcher(const HTTP::Request& req,
		const std::shared_ptr<Interfaces::ICommandDispatcher>& dispatcher)
	{
		if (!dispatcher)
		{
			return MakeResponse(req, HTTP::Status::service_unavailable, ContentTypes::TEXT_PLAIN, "Command dispatcher not available");
		}
		return std::nullopt;
	}

	// Parse the request body as a JSON object into `out`; returns a 400 response on failure.
	inline std::optional<HTTP::Response> ParseJsonObjectBody(const HTTP::Request& req, nlohmann::json& out)
	{
		out = nlohmann::json::parse(req.body(), nullptr, /*allow_exceptions=*/false);
		if (out.is_discarded() || !out.is_object())
		{
			return MakeResponse(req, HTTP::Status::bad_request, ContentTypes::TEXT_PLAIN, "request body must be a JSON object");
		}
		return std::nullopt;
	}
}
// namespace AqualinkAutomate::HTTP
