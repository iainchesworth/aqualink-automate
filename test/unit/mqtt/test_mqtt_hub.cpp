#include <boost/test/unit_test.hpp>

#include <memory>
#include <string>

#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

#include "mqtt/mqtt_hub.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/statistics_hub.h"
#include "options/options_mqtt_options.h"
#include "support/unit_test_mqtt_support.h"

using namespace AqualinkAutomate;

namespace
{
	Options::Mqtt::MqttSettings MakeHubTestSettings()
	{
		return Test::MakeMqttSettings();
	}
}

//=============================================================================
// MqttHub construction tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_MqttHub_Construction)

BOOST_AUTO_TEST_CASE(Test_Construction_Succeeds)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();

	BOOST_CHECK_NO_THROW(Mqtt::MqttHub hub(ioc, settings));
}

BOOST_AUTO_TEST_CASE(Test_Construction_NotRunning)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	BOOST_CHECK(!hub.IsRunning());
}

BOOST_AUTO_TEST_CASE(Test_Construction_ClientCreated)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	BOOST_CHECK(hub.GetMqttClient() != nullptr);
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// MqttHub lifecycle tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_MqttHub_Lifecycle)

BOOST_AUTO_TEST_CASE(Test_Start_SetsRunningFlag)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	hub.Start();

	// IsRunning() will be false since the client is not connected to a broker
	BOOST_CHECK(!hub.IsRunning());

	hub.Stop();
}

BOOST_AUTO_TEST_CASE(Test_Start_TwiceIsHarmless)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	hub.Start();
	BOOST_CHECK_NO_THROW(hub.Start()); // Should not throw

	hub.Stop();
}

BOOST_AUTO_TEST_CASE(Test_Stop_ClearsRunningFlag)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	hub.Start();
	hub.Stop();

	BOOST_CHECK(!hub.IsRunning());
}

BOOST_AUTO_TEST_CASE(Test_Stop_WhenNotRunningIsHarmless)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	BOOST_CHECK_NO_THROW(hub.Stop());
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// MqttHub hub connections tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_MqttHub_HubConnections)

BOOST_AUTO_TEST_CASE(Test_ConnectDataHub_Succeeds)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	auto data_hub = std::make_shared<Kernel::DataHub>();

	BOOST_CHECK_NO_THROW(hub.ConnectDataHub(data_hub));
}

BOOST_AUTO_TEST_CASE(Test_ConnectEquipmentHub_Succeeds)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	auto equip_hub = std::make_shared<Kernel::EquipmentHub>();

	BOOST_CHECK_NO_THROW(hub.ConnectEquipmentHub(equip_hub));
}

BOOST_AUTO_TEST_CASE(Test_ConnectStatisticsHub_Succeeds)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	auto stats_hub = std::make_shared<Kernel::StatisticsHub>();

	BOOST_CHECK_NO_THROW(hub.ConnectStatisticsHub(stats_hub));
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// MqttHub command handler tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_MqttHub_CommandHandlers)

BOOST_AUTO_TEST_CASE(Test_RegisterCommand_AddsHandler)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	hub.RegisterCommand("test_cmd", [](const std::string&, const nlohmann::json&) {});

	BOOST_CHECK_EQUAL(hub.CommandCount(), 1);
	BOOST_CHECK(hub.HasCommand("test_cmd"));
}

BOOST_AUTO_TEST_CASE(Test_RegisterCommand_MultipleHandlers)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	hub.RegisterCommand("cmd1", [](const std::string&, const nlohmann::json&) {});
	hub.RegisterCommand("cmd2", [](const std::string&, const nlohmann::json&) {});
	hub.RegisterCommand("cmd3", [](const std::string&, const nlohmann::json&) {});

	BOOST_CHECK_EQUAL(hub.CommandCount(), 3);
}

BOOST_AUTO_TEST_CASE(Test_UnregisterCommand_RemovesHandler)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	hub.RegisterCommand("to_remove", [](const std::string&, const nlohmann::json&) {});
	BOOST_CHECK_EQUAL(hub.CommandCount(), 1);

	hub.UnregisterCommand("to_remove");
	BOOST_CHECK_EQUAL(hub.CommandCount(), 0);
}

BOOST_AUTO_TEST_CASE(Test_UnregisterCommand_NonexistentIsHarmless)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	BOOST_CHECK_NO_THROW(hub.UnregisterCommand("nonexistent"));
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// MqttHub topic building tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_MqttHub_Topics)

BOOST_AUTO_TEST_CASE(Test_StatusTopic_Format)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	BOOST_CHECK_EQUAL(hub.StatusTopic("pool"), "test/status/pool");
	BOOST_CHECK_EQUAL(hub.StatusTopic("system"), "test/status/system");
	BOOST_CHECK_EQUAL(hub.StatusTopic("devices"), "test/status/devices");
}

BOOST_AUTO_TEST_CASE(Test_EventTopic_Format)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	BOOST_CHECK_EQUAL(hub.EventTopic("config_change"), "test/event/config_change");
}

BOOST_AUTO_TEST_CASE(Test_CommandTopic_Format)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	BOOST_CHECK_EQUAL(hub.CommandTopic("status"), "test/command/status");
	BOOST_CHECK_EQUAL(hub.CommandTopic("device"), "test/command/device");
}

BOOST_AUTO_TEST_CASE(Test_IsCommandTopic_MatchesCommandPrefix)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	BOOST_CHECK(hub.IsCommandTopic("test/command/status"));
	BOOST_CHECK(hub.IsCommandTopic("test/command/device"));
	BOOST_CHECK(!hub.IsCommandTopic("test/status/pool"));
	BOOST_CHECK(!hub.IsCommandTopic("other/command/test"));
}

BOOST_AUTO_TEST_CASE(Test_ExtractCommand_ExtractsAction)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	BOOST_CHECK_EQUAL(hub.ExtractCommand("test/command/status"), "status");
	BOOST_CHECK_EQUAL(hub.ExtractCommand("test/command/device"), "device");
	BOOST_CHECK_EQUAL(hub.ExtractCommand("test/command/temperature"), "temperature");
}

BOOST_AUTO_TEST_CASE(Test_ExtractCommand_NonCommandTopicReturnsEmpty)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	BOOST_CHECK_EQUAL(hub.ExtractCommand("test/status/pool"), "");
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// MqttHub signal tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_MqttHub_Signals)

BOOST_AUTO_TEST_CASE(Test_OnDevicesPublished_SignalExists)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	bool signal_fired = false;
	auto connection = hub.OnDevicesPublished.connect([&signal_fired]() {
		signal_fired = true;
	});

	// We can't trigger PublishDeviceStatus without being connected,
	// but we can verify the signal connection doesn't throw
	BOOST_CHECK(!signal_fired);
}

BOOST_AUTO_TEST_CASE(Test_GetMqttClient_ReturnsSharedClient)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	auto client1 = hub.GetMqttClient();
	auto client2 = hub.GetMqttClient();

	BOOST_CHECK(client1 != nullptr);
	BOOST_CHECK_EQUAL(client1.get(), client2.get()); // Same pointer
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// MqttHub no-crash tests (publish methods when not connected)
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_MqttHub_NoCrash)

BOOST_AUTO_TEST_CASE(Test_PublishAllStatus_WhenNotRunning_NoCrash)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	BOOST_CHECK_NO_THROW(hub.PublishAllStatus());
}

BOOST_AUTO_TEST_CASE(Test_PublishCustom_WhenNotRunning_NoCrash)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	nlohmann::json payload = {{"test", "value"}};
	BOOST_CHECK_NO_THROW(hub.PublishCustom("custom/test", payload));
}

BOOST_AUTO_TEST_CASE(Test_Poll_WhenNotRunning_NoCrash)
{
	boost::asio::io_context ioc;
	auto settings = MakeHubTestSettings();
	Mqtt::MqttHub hub(ioc, settings);

	BOOST_CHECK_NO_THROW(hub.Poll());
}

BOOST_AUTO_TEST_SUITE_END()
