#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

#include "interfaces/ihub.h"
#include "kernel/temperature.h"

namespace AqualinkAutomate::Kernel
{

	// User/admin preferences (as opposed to boot-only CLI infrastructure).
	//
	// The typed fields below are READ LIVE by the relevant services
	// (AlertMonitor, HistoryService, the webhook sink), so a change via the
	// preferences API takes effect without a restart.  They are seeded from the
	// CLI/config at boot (so existing deployments are unchanged), then overridden
	// by the persisted preferences file if present.
	//
	// UiPreferences is an opaque JSON blob the backend never interprets (e.g. the
	// chemistry gauge bands); it simply round-trips through the API + file so the
	// UI has a shared, server-persisted home for its display preferences.
	class PreferencesHub : public Interfaces::IHub
	{
	public:
		PreferencesHub();
		~PreferencesHub() override = default;

	public:
		TemperatureUnits Temperature_DisplayUnits{ TemperatureUnits::Celsius };

		std::uint32_t AlertSaltLowPpm{ 2600 };
		std::uint32_t AlertCommsTimeoutSeconds{ 60 };
		std::string AlertWebhookUrl;
		std::uint32_t HistoryRetentionDays{ 90 };

		// User-friendly display names keyed by the device's canonical label
		// (e.g. {"Aux1": "Pool Light"}). The canonical label is unchanged (it
		// still drives dispatch / MQTT / HA); this only feeds a display_label.
		nlohmann::json LabelOverrides{ nlohmann::json::object() };

		nlohmann::json UiPreferences{ nlohmann::json::object() };
	};

}
// namespace AqualinkAutomate::Kernel
