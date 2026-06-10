#include <filesystem>
#include <string>
#include <system_error>

#include <boost/test/unit_test.hpp>

#include <nlohmann/json.hpp>

#include "kernel/preferences_hub.h"
#include "options/options_preferences_options.h"
#include "preferences/preferences_service.h"

#include "support/unit_test_hublocatorinjector.h"

using namespace AqualinkAutomate;

BOOST_FIXTURE_TEST_SUITE(TestSuite_PreferencesService, Test::HubLocatorInjector)

BOOST_AUTO_TEST_CASE(Seed_SetsRuntimeFields)
{
	Options::Preferences::PreferencesSettings settings;   // no file
	Preferences::PreferencesService service(*this, settings);

	service.Seed(3000, 30, "https://hook.example/x", 45);

	auto json = service.ToJson();
	BOOST_CHECK_EQUAL(json["alert"]["salt_low_ppm"], 3000);
	BOOST_CHECK_EQUAL(json["alert"]["comms_timeout_seconds"], 30);
	BOOST_CHECK_EQUAL(json["alert"]["webhook_url"], "https://hook.example/x");
	BOOST_CHECK_EQUAL(json["history"]["retention_days"], 45);
}

BOOST_AUTO_TEST_CASE(ApplyJson_ValidUpdatesHubLive)
{
	Options::Preferences::PreferencesSettings settings;
	Preferences::PreferencesService service(*this, settings);

	nlohmann::json update = {
		{ "temperature_units", "Fahrenheit" },
		{ "alert", { { "salt_low_ppm", 2800 }, { "webhook_url", "http://x.local/hook" } } },
		{ "history", { { "retention_days", 14 } } },
		{ "ui", { { "chemistryBands", { { "ph", { { "goodMin", 7.4 } } } } } } },
	};

	std::string error;
	BOOST_REQUIRE_MESSAGE(service.ApplyJson(update, error), error);

	auto hub = Find<Kernel::PreferencesHub>();
	BOOST_CHECK(hub->Temperature_DisplayUnits == Kernel::TemperatureUnits::Fahrenheit);
	BOOST_CHECK_EQUAL(hub->AlertSaltLowPpm, 2800u);
	BOOST_CHECK_EQUAL(hub->AlertWebhookUrl, "http://x.local/hook");
	BOOST_CHECK_EQUAL(hub->HistoryRetentionDays, 14u);
	// Opaque UI blob round-trips.
	BOOST_CHECK_EQUAL(hub->UiPreferences["chemistryBands"]["ph"]["goodMin"], 7.4);
}

BOOST_AUTO_TEST_CASE(ApplyJson_RejectsBadValues)
{
	Options::Preferences::PreferencesSettings settings;
	Preferences::PreferencesService service(*this, settings);
	std::string error;

	BOOST_CHECK(!service.ApplyJson(nlohmann::json{ { "temperature_units", "Kelvin" } }, error));
	BOOST_CHECK(!service.ApplyJson(nlohmann::json{ { "alert", { { "salt_low_ppm", 99999 } } } }, error));
	BOOST_CHECK(!service.ApplyJson(nlohmann::json{ { "alert", { { "comms_timeout_seconds", 0 } } } }, error));
	BOOST_CHECK(!service.ApplyJson(nlohmann::json{ { "alert", { { "webhook_url", "not-a-url" } } } }, error));
	BOOST_CHECK(!service.ApplyJson(nlohmann::json{ { "history", { { "retention_days", 0 } } } }, error));
}

BOOST_AUTO_TEST_CASE(ApplyJson_RejectionLeavesHubUnchanged)
{
	Options::Preferences::PreferencesSettings settings;
	Preferences::PreferencesService service(*this, settings);
	service.Seed(2600, 60, "", 90);

	std::string error;
	// salt is valid but comms is invalid -> the whole apply must be rejected.
	BOOST_CHECK(!service.ApplyJson(nlohmann::json{ { "alert", { { "salt_low_ppm", 1000 }, { "comms_timeout_seconds", 0 } } } }, error));

	BOOST_CHECK_EQUAL(Find<Kernel::PreferencesHub>()->AlertSaltLowPpm, 2600u);  // unchanged
}

BOOST_AUTO_TEST_CASE(FileRoundTrip_PersistsAndReloads)
{
	const auto path = (std::filesystem::temp_directory_path() / "aqualink_prefs_test.json").string();
	std::error_code ec;
	std::filesystem::remove(path, ec);

	Options::Preferences::PreferencesSettings settings;
	settings.preferences_file = path;

	{
		Preferences::PreferencesService service(*this, settings);
		std::string error;
		BOOST_REQUIRE(service.ApplyJson(nlohmann::json{ { "alert", { { "salt_low_ppm", 3300 } } } }, error));  // saves
	}

	// A fresh service + fresh hub loads the persisted value.
	Test::HubLocatorInjector fresh_locator;
	Preferences::PreferencesService reloaded(fresh_locator, settings);
	reloaded.Start();
	BOOST_CHECK_EQUAL(fresh_locator.Find<Kernel::PreferencesHub>()->AlertSaltLowPpm, 3300u);

	std::filesystem::remove(path, ec);
	std::filesystem::remove(path + ".tmp", ec);
}

BOOST_AUTO_TEST_SUITE_END()
