#include <boost/test/unit_test.hpp>

#include <boost/asio/io_context.hpp>
#include <crow/app.h>
#include <nlohmann/json.hpp>

#include "http/webroute_jandyequipment.h"

#include "support/unit_test_onetouchdevice.h"

using namespace AqualinkAutomate;

BOOST_FIXTURE_TEST_SUITE(HttpRoutes_ApiJandyEquipment, Test::OneTouchDevice)

BOOST_AUTO_TEST_CASE(UninitialisedJandyConfig)
{
	crow::SimpleApp http_server;

	HTTP::WebRoute_JandyEquipment route_je(http_server, Equipment());

	BOOST_REQUIRE_NO_THROW(http_server.validate());

	{
		HTTP::Request req;
		HTTP::Response res;

		req.url = "/api/equipment";

		http_server.handle_full(req, res);

		BOOST_CHECK_EQUAL(200, res.code);

		nlohmann::json json_response = nlohmann::json::parse(res.body);

		BOOST_REQUIRE(json_response.contains("buttons"));
		BOOST_REQUIRE(json_response.contains("devices"));
		BOOST_REQUIRE(json_response.contains("stats"));
		BOOST_REQUIRE(json_response.contains("version"));

		BOOST_CHECK(json_response["buttons"].is_null());

		BOOST_REQUIRE(json_response["devices"].contains("auxillaries"));
		BOOST_REQUIRE(json_response["devices"].contains("heaters"));
		BOOST_REQUIRE(json_response["devices"].contains("pumps"));

		BOOST_CHECK(json_response["devices"]["auxillaries"].is_null());
		BOOST_CHECK(json_response["devices"]["heaters"].is_null());
		BOOST_CHECK(json_response["devices"]["pumps"].is_null());

		BOOST_REQUIRE(json_response["version"].contains("fw_revision"));
		BOOST_REQUIRE(json_response["version"].contains("model_number"));

		BOOST_CHECK_EQUAL("", json_response["version"]["fw_revision"]);
		BOOST_CHECK_EQUAL("", json_response["version"]["model_number"]);
	}

}

BOOST_AUTO_TEST_CASE(InitialisedJandyConfig)
{
	crow::SimpleApp http_server;

	HTTP::WebRoute_JandyEquipment route_je(http_server, Equipment());

	http_server.validate();

	InitialiseOneTouchDevice();
	EquipmentOnOff_Page1();
	EquipmentOnOff_Page2();
	EquipmentOnOff_Page3();

	auto check_for_device = [](const auto& label, const auto& state, const auto& device_vec) -> bool
	{
		bool was_found = false;

		for (const auto& device : device_vec)
		{
			if (!device.contains("label"))
			{
				// No label...ignore.
			}
			else if (!device.contains("state"))
			{
				// No state...ignore.
			}
			else if (label != device.at("label"))
			{
				// Label doesn't match...ignore.
			}
			else if (state != device.at("state"))
			{
				// State doesn't match...ignore.
			}
			else
			{
				was_found = true;
				break;
			}
		}

		return was_found;
	};

	{
		HTTP::Request req;
		HTTP::Response res;

		req.url = "/api/equipment";

		http_server.handle_full(req, res);

		BOOST_CHECK_EQUAL(200, res.code);

		nlohmann::json json_response = nlohmann::json::parse(res.body);

		BOOST_REQUIRE(json_response.contains("buttons"));
		BOOST_REQUIRE(json_response.contains("devices"));
		BOOST_REQUIRE(json_response.contains("stats"));
		BOOST_REQUIRE(json_response.contains("version"));

		BOOST_CHECK(json_response["buttons"].is_null());

		BOOST_REQUIRE(json_response["devices"].contains("auxillaries"));
		BOOST_REQUIRE(json_response["devices"].contains("heaters"));
		BOOST_REQUIRE(json_response["devices"].contains("pumps"));

		BOOST_CHECK_EQUAL(15, json_response["devices"]["auxillaries"].size());
		BOOST_CHECK_EQUAL(2, json_response["devices"]["heaters"].size());
		BOOST_CHECK_EQUAL(2, json_response["devices"]["pumps"].size());

		BOOST_CHECK(check_for_device("Aux1", "Off", json_response["devices"]["auxillaries"]));
		BOOST_CHECK(check_for_device("Aux2", "Off", json_response["devices"]["auxillaries"]));
		BOOST_CHECK(check_for_device("Aux3", "On", json_response["devices"]["auxillaries"]));
		BOOST_CHECK(check_for_device("Aux4", "Off", json_response["devices"]["auxillaries"]));
		BOOST_CHECK(check_for_device("Aux5", "Off", json_response["devices"]["auxillaries"]));
		BOOST_CHECK(check_for_device("Aux6", "Off", json_response["devices"]["auxillaries"]));
		BOOST_CHECK(!check_for_device("Aux7", "Off", json_response["devices"]["auxillaries"]));
		BOOST_CHECK(!check_for_device("Aux8", "N/A", json_response["devices"]["auxillaries"]));
		BOOST_CHECK(check_for_device("Aux B1", "Off", json_response["devices"]["auxillaries"]));
		BOOST_CHECK(check_for_device("Aux B2", "Off", json_response["devices"]["auxillaries"]));
		BOOST_CHECK(check_for_device("Aux B3", "Off", json_response["devices"]["auxillaries"]));
		BOOST_CHECK(check_for_device("Aux B4", "Off", json_response["devices"]["auxillaries"]));
		BOOST_CHECK(check_for_device("Aux B5", "On", json_response["devices"]["auxillaries"]));
		BOOST_CHECK(check_for_device("Aux B6", "Off", json_response["devices"]["auxillaries"]));
		BOOST_CHECK(check_for_device("Aux B7", "Off", json_response["devices"]["auxillaries"]));
		BOOST_CHECK(check_for_device("Aux B8", "Off", json_response["devices"]["auxillaries"]));
		BOOST_CHECK(check_for_device("Extra Aux", "Off", json_response["devices"]["auxillaries"]));
		BOOST_CHECK(check_for_device("Pool Heat", "Enabled", json_response["devices"]["heaters"]));
		BOOST_CHECK(check_for_device("Spa Heat", "Off", json_response["devices"]["heaters"]));
		BOOST_CHECK(!check_for_device("Solar Heat", "N/A", json_response["devices"]["heaters"]));
		BOOST_CHECK(!check_for_device("Heat Pump", "N/A", json_response["devices"]["heaters"]));
		BOOST_CHECK(check_for_device("Pool Pump", "Unknown", json_response["devices"]["pumps"]));
		BOOST_CHECK(check_for_device("Spa Pump", "Running", json_response["devices"]["pumps"]));
		BOOST_CHECK(!check_for_device("Filter Pump", "N/A", json_response["devices"]["pumps"]));

		// Check for mis-parsing of the all off command.
		BOOST_CHECK(!check_for_device("All Off", "Unknown", json_response["devices"]["auxillaries"]));
		BOOST_CHECK(!check_for_device("All Off", "Unknown", json_response["devices"]["heaters"]));
		BOOST_CHECK(!check_for_device("All Off", "Unknown", json_response["devices"]["pumps"]));

		BOOST_REQUIRE(json_response["version"].contains("fw_revision"));
		BOOST_REQUIRE(json_response["version"].contains("model_number"));

		BOOST_CHECK_EQUAL("REV T.0.1", json_response["version"]["fw_revision"]);
		BOOST_CHECK_EQUAL("B0029221", json_response["version"]["model_number"]);
	}
}

BOOST_AUTO_TEST_SUITE_END()
