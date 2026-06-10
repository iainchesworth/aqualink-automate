#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

namespace AqualinkAutomate::Scheduling
{

	// Schedule actions mirror ICommandDispatcher capabilities exactly — no new
	// command types are invented. Wire names are snake_case.
	enum class ActionType : std::uint8_t
	{
		ButtonOn,            // button_on            (target = device label)
		ButtonOff,           // button_off           (target = device label)
		ButtonToggle,        // button_toggle        (target = device label)
		PoolSetpoint,        // pool_setpoint        (value 1..104)
		SpaSetpoint,         // spa_setpoint         (value 1..104)
		ChlorinatorPercent,  // chlorinator_percent  (value 0..100)
		CirculationMode,     // circulation_mode     (target = mode string)
	};

	std::string_view ActionTypeToString(ActionType type);
	std::optional<ActionType> ActionTypeFromString(std::string_view text);

	struct Action
	{
		ActionType type{ ActionType::ButtonToggle };
		std::string target;   // device label (button_*) or circulation mode string
		int value{ 0 };       // setpoint / chlorinator percentage
	};

	struct Schedule
	{
		std::string uuid;
		std::string name;
		bool enabled{ true };
		std::uint8_t days_of_week{ 0 };  // bitmask, bit0 = Monday .. bit6 = Sunday
		int hour{ 0 };                   // 0..23 (local wall-clock)
		int minute{ 0 };                 // 0..59
		Action action;
	};

	// Serialise to the wire/file JSON shape (time as "HH:MM").
	nlohmann::json ToJson(const Schedule& schedule);

	// Parse + validate. Returns std::nullopt and sets `error` on any invalid
	// field (bad time, unknown action, out-of-range value). A present "uuid" is
	// preserved; an absent one yields an empty uuid (the caller assigns one).
	std::optional<Schedule> FromJson(const nlohmann::json& json, std::string& error);

}
// namespace AqualinkAutomate::Scheduling
