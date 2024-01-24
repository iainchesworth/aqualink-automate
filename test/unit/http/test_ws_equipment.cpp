#include <boost/test/unit_test.hpp>

#include <nlohmann/json.hpp>

#include "http/websocket_event.h"
#include "http/websocket_equipment.h"
#include "http/server/routing/routing.h"
#include "kernel/hub_events/data_hub_config_event.h"
#include "kernel/hub_events/data_hub_config_event_chemistry.h"
#include "kernel/hub_events/data_hub_config_event_temperature.h"

#include "support/unit_test_httprequestresponse.h"
#include "support/unit_test_onetouchdevice.h"
#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate;

BOOST_FIXTURE_TEST_SUITE(TestSuite_WebsocketRoutes_WsEquipment, Test::OneTouchDevice)

BOOST_AUTO_TEST_CASE(Test_WebsocketRoutes_WsEquipment_WebSocket_ChemistryEventConversion)
{
	{
		auto config_event_null = std::make_shared<Kernel::DataHub_ConfigEvent_Chemistry>();
		auto config_event_base = std::dynamic_pointer_cast<Kernel::DataHub_ConfigEvent>(config_event_null);
		BOOST_REQUIRE(nullptr != config_event_base);
		HTTP::WebSocket_Event wse1(config_event_base);

		BOOST_CHECK_EQUAL(HTTP::WebSocket_EventTypes::ChemistryUpdate, wse1.Type());
		auto wse1_string = wse1.Payload();
		auto wse1_json = nlohmann::json::parse(wse1_string);
		auto wse1_json_type = wse1_json["type"];
		auto wse1_json_payload = wse1_json["payload"];
		BOOST_CHECK_EQUAL("ChemistryUpdate", wse1_json_type);
		BOOST_CHECK(wse1_json_payload.is_null());
	}

	{
		auto config_event_chem = std::make_shared<Kernel::DataHub_ConfigEvent_Chemistry>();
		BOOST_REQUIRE(nullptr != config_event_chem);
		config_event_chem->ORP(650);
		config_event_chem->pH(7.5f);
		HTTP::WebSocket_Event wse2(config_event_chem);

		BOOST_CHECK_EQUAL(HTTP::WebSocket_EventTypes::ChemistryUpdate, wse2.Type());
		auto wse2_string = wse2.Payload();
		auto wse2_json = nlohmann::json::parse(wse2_string);
		auto wse2_json_type = wse2_json["type"];
		auto wse2_json_payload = wse2_json["payload"];
		BOOST_CHECK_EQUAL("ChemistryUpdate", wse2_json_type);
		BOOST_CHECK(wse2_json["payload"].contains("orp"));
		BOOST_CHECK(wse2_json["payload"]["orp"].is_number());
		BOOST_CHECK_EQUAL(650, wse2_json["payload"]["orp"]);
		BOOST_CHECK(wse2_json["payload"].contains("ph"));
		BOOST_CHECK(wse2_json["payload"]["ph"].is_number_float());
		BOOST_CHECK_EQUAL(7.5f, wse2_json["payload"]["ph"]);
	}
}

BOOST_AUTO_TEST_CASE(Test_WebsocketRoutes_WsEquipment_WebSocket_TemperatureEventConversion)
{
	{
		auto config_event_null = std::make_shared<Kernel::DataHub_ConfigEvent_Temperature>();
		auto config_event_base = std::dynamic_pointer_cast<Kernel::DataHub_ConfigEvent>(config_event_null);
		BOOST_REQUIRE(nullptr != config_event_base);
		HTTP::WebSocket_Event wse1(config_event_base);

		BOOST_CHECK_EQUAL(HTTP::WebSocket_EventTypes::TemperatureUpdate, wse1.Type());
		auto wse1_string = wse1.Payload();
		auto wse1_json = nlohmann::json::parse(wse1_string);
		auto wse1_json_type = wse1_json["type"];
		auto wse1_json_payload = wse1_json["payload"];
		BOOST_CHECK_EQUAL("TemperatureUpdate", wse1_json_type);
		BOOST_CHECK(wse1_json_payload.is_null());
	}

	{
		auto config_event_temp = std::make_shared<Kernel::DataHub_ConfigEvent_Temperature>();
		BOOST_REQUIRE(nullptr != config_event_temp);
		Utility::TemperatureStringConverter pool_temp("Pool        90`F");
		config_event_temp->PoolTemp(pool_temp().value()); // Make sure to use the right separator character --> `
		HTTP::WebSocket_Event wse2(config_event_temp);
	
		BOOST_CHECK_EQUAL(HTTP::WebSocket_EventTypes::TemperatureUpdate, wse2.Type());
		auto wse2_string = wse2.Payload();
		auto wse2_json = nlohmann::json::parse(wse2_string);
		auto wse2_json_type = wse2_json["type"];
		auto wse2_json_payload = wse2_json["payload"];
		BOOST_CHECK_EQUAL("TemperatureUpdate", wse2_json_type);
		BOOST_CHECK(wse2_json["payload"].contains("pool_temp"));
		BOOST_CHECK(wse2_json["payload"]["pool_temp"].is_string());
		BOOST_CHECK_EQUAL("90\u00B0F", wse2_json["payload"]["pool_temp"]); // Make sure to use the right separator character --> \u00B0
	}

	{
		auto config_event_temp = std::make_shared<Kernel::DataHub_ConfigEvent_Temperature>();
		BOOST_REQUIRE(nullptr != config_event_temp);
		config_event_temp->PoolTemp(Utility::TemperatureStringConverter("Pool        21`C")().value()); // Make sure to use the right separator character --> `
		config_event_temp->SpaTemp(Utility::TemperatureStringConverter("Spa         39`C")().value()); // Make sure to use the right separator character --> `
		config_event_temp->AirTemp(Utility::TemperatureStringConverter("Air         16`C")().value()); // Make sure to use the right separator character --> `
		HTTP::WebSocket_Event wse3(config_event_temp);

		BOOST_CHECK_EQUAL(HTTP::WebSocket_EventTypes::TemperatureUpdate, wse3.Type());
		auto wse3_string = wse3.Payload();
		auto wse3_json = nlohmann::json::parse(wse3_string);
		auto wse3_json_type = wse3_json["type"];
		auto wse3_json_payload = wse3_json["payload"];
		BOOST_CHECK_EQUAL("TemperatureUpdate", wse3_json_type);
		BOOST_CHECK(wse3_json["payload"].contains("pool_temp"));
		BOOST_CHECK(wse3_json["payload"]["pool_temp"].is_string());
		BOOST_CHECK("21\u00B0C" == wse3_json["payload"]["pool_temp"]); // Make sure to use the right separator character --> \u00B0
		BOOST_CHECK(wse3_json["payload"].contains("spa_temp"));
		BOOST_CHECK(wse3_json["payload"]["spa_temp"].is_string());
		BOOST_CHECK("39\u00B0C" == wse3_json["payload"]["spa_temp"]); // Make sure to use the right separator character --> \u00B0
		BOOST_CHECK(wse3_json["payload"].contains("air_temp"));
		BOOST_CHECK(wse3_json["payload"]["air_temp"].is_string());
		BOOST_CHECK("16\u00B0C" == wse3_json["payload"]["air_temp"]); // Make sure to use the right separator character --> \u00B0
	}
}

BOOST_AUTO_TEST_CASE(Test_WebsocketRoutes_WsEquipment_WebSocket_PublishChemistryUpdate)
{
	boost::asio::io_context io_context;
	boost::beast::error_code ec;

	InitialiseOneTouchDevice();
	EquipmentOnOff_Page1();
	EquipmentOnOff_Page2();
	EquipmentOnOff_Page3();

	auto route_ws_equipment = std::make_unique<HTTP::WebSocket_Equipment>(*this);
	BOOST_REQUIRE(nullptr != route_ws_equipment);
	HTTP::Routing::Add(std::move(route_ws_equipment));

	auto&& tro = Test::PerformHttpWsUpgradeResponse(io_context, "/ws/equipment");

	std::array<uint8_t, 1024> websocket_buffer;
	BOOST_TEST_REQUIRE(0 == tro->local_stream.buffer().size());

	{
		DataHub().ORP(650);
		io_context.poll(); // ASYNC_READ

		auto size = tro->local_stream.buffer().size();
		const auto bytes_read = tro->local_stream.read_some(boost::asio::buffer(websocket_buffer), ec);
		BOOST_TEST_REQUIRE(0 == ec.value());
		BOOST_TEST_REQUIRE(0 != bytes_read);

		nlohmann::json wse_json;
		BOOST_REQUIRE_NO_THROW(wse_json = nlohmann::json::parse(websocket_buffer.data() + 2, websocket_buffer.data() + bytes_read));

		BOOST_REQUIRE(wse_json.contains("type"));
		BOOST_CHECK_EQUAL("ChemistryUpdate", wse_json["type"]);
		BOOST_REQUIRE(wse_json.contains("payload"));
		BOOST_REQUIRE(wse_json["payload"].contains("orp"));
		BOOST_CHECK_EQUAL(650, wse_json["payload"]["orp"]);
	}

	{
		DataHub().pH(7.5);
		io_context.poll(); // ASYNC_READ

		auto size = tro->local_stream.buffer().size();
		const auto bytes_read = tro->local_stream.read_some(boost::asio::buffer(websocket_buffer), ec);
		BOOST_TEST_REQUIRE(0 == ec.value());
		BOOST_TEST_REQUIRE(0 != bytes_read);

		nlohmann::json wse_json;
		BOOST_REQUIRE_NO_THROW(wse_json = nlohmann::json::parse(websocket_buffer.data() + 2, websocket_buffer.data() + bytes_read));

		BOOST_REQUIRE(wse_json.contains("type"));
		BOOST_CHECK_EQUAL("ChemistryUpdate", wse_json["type"]);
		BOOST_REQUIRE(wse_json.contains("payload"));
		BOOST_REQUIRE(wse_json["payload"].contains("ph"));
		BOOST_CHECK_EQUAL(7.5, wse_json["payload"]["ph"]);
	}

	{
		DataHub().SaltLevel(4000 * ppm);
		io_context.poll(); // ASYNC_READ

		auto size = tro->local_stream.buffer().size();
		const auto bytes_read = tro->local_stream.read_some(boost::asio::buffer(websocket_buffer), ec);
		BOOST_TEST_REQUIRE(0 == ec.value());
		BOOST_TEST_REQUIRE(0 != bytes_read);

		nlohmann::json wse_json;
		BOOST_REQUIRE_NO_THROW(wse_json = nlohmann::json::parse(websocket_buffer.data() + 2, websocket_buffer.data() + bytes_read));

		BOOST_REQUIRE(wse_json.contains("type"));
		BOOST_CHECK_EQUAL("ChemistryUpdate", wse_json["type"]);
		BOOST_REQUIRE(wse_json.contains("payload"));
		BOOST_REQUIRE(wse_json["payload"].contains("salt_level"));
		BOOST_CHECK_EQUAL(4000, wse_json["payload"]["salt_level"]);
	}
}

BOOST_AUTO_TEST_CASE(Test_WebsocketRoutes_WsEquipment_WebSocket_PublishTemperatureUpdate)
{
	boost::asio::io_context io_context;
	boost::beast::error_code ec;

	InitialiseOneTouchDevice();
	EquipmentOnOff_Page1();
	EquipmentOnOff_Page2();
	EquipmentOnOff_Page3();

	auto route_ws_equipment = std::make_unique<HTTP::WebSocket_Equipment>(*this);
	BOOST_REQUIRE(nullptr != route_ws_equipment);
	HTTP::Routing::Add(std::move(route_ws_equipment));

	auto&& tro = Test::PerformHttpWsUpgradeResponse(io_context, "/ws/equipment");

	std::array<uint8_t, 1024> websocket_buffer;
	BOOST_TEST_REQUIRE(0 == tro->local_stream.buffer().size());

	{
		DataHub().PoolTemp(Utility::TemperatureStringConverter("Pool        38`C")().value());
		io_context.poll(); // ASYNC_READ

		auto size = tro->local_stream.buffer().size();
		const auto bytes_read = tro->local_stream.read_some(boost::asio::buffer(websocket_buffer), ec);
		BOOST_TEST_REQUIRE(0 == ec.value());
		BOOST_TEST_REQUIRE(0 != bytes_read);

		nlohmann::json wse_json;
		BOOST_REQUIRE_NO_THROW(wse_json = nlohmann::json::parse(websocket_buffer.data() + 2, websocket_buffer.data() + bytes_read));

		BOOST_REQUIRE(wse_json.contains("type"));
		BOOST_CHECK_EQUAL("TemperatureUpdate", wse_json["type"]);
		BOOST_REQUIRE(wse_json.contains("payload"));
		BOOST_REQUIRE(wse_json["payload"].contains("pool_temp"));
		BOOST_CHECK_EQUAL("38\u00B0C", wse_json["payload"]["pool_temp"]);
	}
}

BOOST_AUTO_TEST_SUITE_END()
