#include <boost/test/unit_test.hpp>

#include <memory>
#include <string>

#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

#include "mqtt/mqtt_integration.h"
#include "mqtt/mqtt_hub.h"
#include "mqtt/mqtt_client.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/hub_locator.h"
#include "kernel/statistics_hub.h"
#include "options/options_mqtt_options.h"
#include "support/unit_test_mqtt_support.h"

using namespace AqualinkAutomate;

namespace
{
	Options::Mqtt::MqttSettings MakeEnabledSettings()
	{
		return Test::MakeMqttSettings();
	}

	Options::Mqtt::MqttSettings MakeDisabledSettings()
	{
		Options::Mqtt::MqttSettings s;
		s.enabled = false;
		return s;
	}

	Options::Mqtt::MqttSettings MakeHaEnabledSettings()
	{
		auto s = Test::MakeMqttSettings();
		s.home_assistant_enabled = true;
		s.ha_discovery_prefix = "homeassistant";
		s.ha_device_id = "aqualink_test";
		return s;
	}
}

//=============================================================================
// MqttIntegration construction tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_MqttIntegration_Construction)

BOOST_AUTO_TEST_CASE(Test_Construction_WhenEnabled_Succeeds)
{
	boost::asio::io_context ioc;
	auto settings = MakeEnabledSettings();

	BOOST_CHECK_NO_THROW(Mqtt::MqttIntegration integration(ioc, settings));
}

BOOST_AUTO_TEST_CASE(Test_Construction_WhenDisabled_Succeeds)
{
	boost::asio::io_context ioc;
	auto settings = MakeDisabledSettings();

	BOOST_CHECK_NO_THROW(Mqtt::MqttIntegration integration(ioc, settings));
}

BOOST_AUTO_TEST_CASE(Test_Construction_WhenEnabled_CreatesHub)
{
	boost::asio::io_context ioc;
	auto settings = MakeEnabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	BOOST_CHECK(integration.GetMqttHub() != nullptr);
}

BOOST_AUTO_TEST_CASE(Test_Construction_WhenDisabled_NoHub)
{
	boost::asio::io_context ioc;
	auto settings = MakeDisabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	BOOST_CHECK(integration.GetMqttHub() == nullptr);
}

BOOST_AUTO_TEST_CASE(Test_Construction_WithHomeAssistant_ConfiguresLWT)
{
	boost::asio::io_context ioc;
	auto settings = MakeHaEnabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	auto hub = integration.GetMqttHub();
	BOOST_REQUIRE(hub != nullptr);

	auto client = hub->GetMqttClient();
	BOOST_REQUIRE(client != nullptr);

	// When HA is enabled, LWT should be configured on the client
	auto& will = client->GetWill();
	BOOST_REQUIRE(will.has_value());
	BOOST_CHECK_EQUAL(will->topic, "test/status/availability");
	BOOST_CHECK_EQUAL(will->payload, "offline");
	BOOST_CHECK(will->retain);
}

BOOST_AUTO_TEST_CASE(Test_Construction_WithoutHomeAssistant_NoLWT)
{
	boost::asio::io_context ioc;
	auto settings = MakeEnabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	auto client = integration.GetMqttHub()->GetMqttClient();
	BOOST_REQUIRE(client != nullptr);

	auto& will = client->GetWill();
	BOOST_CHECK(!will.has_value());
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// MqttIntegration lifecycle tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_MqttIntegration_Lifecycle)

BOOST_AUTO_TEST_CASE(Test_IsEnabled_WhenEnabled)
{
	boost::asio::io_context ioc;
	auto settings = MakeEnabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	BOOST_CHECK(integration.IsEnabled());
}

BOOST_AUTO_TEST_CASE(Test_IsEnabled_WhenDisabled)
{
	boost::asio::io_context ioc;
	auto settings = MakeDisabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	BOOST_CHECK(!integration.IsEnabled());
}

BOOST_AUTO_TEST_CASE(Test_IsRunning_WhenNotStarted)
{
	boost::asio::io_context ioc;
	auto settings = MakeEnabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	// Not started, not connected -> not running
	BOOST_CHECK(!integration.IsRunning());
}

BOOST_AUTO_TEST_CASE(Test_IsRunning_WhenDisabled)
{
	boost::asio::io_context ioc;
	auto settings = MakeDisabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	BOOST_CHECK(!integration.IsRunning());
}

BOOST_AUTO_TEST_CASE(Test_Start_WhenEnabled_NoCrash)
{
	boost::asio::io_context ioc;
	auto settings = MakeEnabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	BOOST_CHECK_NO_THROW(integration.Start());

	integration.Stop();
}

BOOST_AUTO_TEST_CASE(Test_Start_WhenDisabled_NoCrash)
{
	boost::asio::io_context ioc;
	auto settings = MakeDisabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	BOOST_CHECK_NO_THROW(integration.Start());
}

BOOST_AUTO_TEST_CASE(Test_Stop_WhenNotStarted_NoCrash)
{
	boost::asio::io_context ioc;
	auto settings = MakeEnabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	BOOST_CHECK_NO_THROW(integration.Stop());
}

BOOST_AUTO_TEST_CASE(Test_Stop_WhenDisabled_NoCrash)
{
	boost::asio::io_context ioc;
	auto settings = MakeDisabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	BOOST_CHECK_NO_THROW(integration.Stop());
}

BOOST_AUTO_TEST_CASE(Test_Poll_WhenEnabled_NoCrash)
{
	boost::asio::io_context ioc;
	auto settings = MakeEnabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	BOOST_CHECK_NO_THROW(integration.Poll());
}

BOOST_AUTO_TEST_CASE(Test_Poll_WhenDisabled_NoCrash)
{
	boost::asio::io_context ioc;
	auto settings = MakeDisabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	BOOST_CHECK_NO_THROW(integration.Poll());
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// MqttIntegration hub connection tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_MqttIntegration_HubConnections)

BOOST_AUTO_TEST_CASE(Test_ConnectHubs_Individual_Succeeds)
{
	boost::asio::io_context ioc;
	auto settings = MakeEnabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	auto data_hub = std::make_shared<Kernel::DataHub>();
	auto equip_hub = std::make_shared<Kernel::EquipmentHub>();
	auto stats_hub = std::make_shared<Kernel::StatisticsHub>();

	BOOST_CHECK_NO_THROW(integration.ConnectHubs(data_hub, equip_hub, stats_hub));
}

BOOST_AUTO_TEST_CASE(Test_ConnectHubs_WithNullHubs_NoCrash)
{
	boost::asio::io_context ioc;
	auto settings = MakeEnabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	BOOST_CHECK_NO_THROW(integration.ConnectHubs(nullptr, nullptr, nullptr));
}

BOOST_AUTO_TEST_CASE(Test_ConnectHubs_WhenDisabled_NoCrash)
{
	boost::asio::io_context ioc;
	auto settings = MakeDisabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	auto data_hub = std::make_shared<Kernel::DataHub>();
	auto equip_hub = std::make_shared<Kernel::EquipmentHub>();
	auto stats_hub = std::make_shared<Kernel::StatisticsHub>();

	BOOST_CHECK_NO_THROW(integration.ConnectHubs(data_hub, equip_hub, stats_hub));
}

BOOST_AUTO_TEST_CASE(Test_ConnectHubs_ViaHubLocator_Succeeds)
{
	boost::asio::io_context ioc;
	auto settings = MakeEnabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	auto data_hub = std::make_shared<Kernel::DataHub>();
	auto equip_hub = std::make_shared<Kernel::EquipmentHub>();
	auto stats_hub = std::make_shared<Kernel::StatisticsHub>();

	Kernel::HubLocator locator;
	locator.Register(data_hub).Register(equip_hub).Register(stats_hub);

	BOOST_CHECK_NO_THROW(integration.ConnectHubs(locator));
}

BOOST_AUTO_TEST_CASE(Test_ConnectHubs_WithHomeAssistant_ConnectsDataHub)
{
	boost::asio::io_context ioc;
	auto settings = MakeHaEnabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	auto data_hub = std::make_shared<Kernel::DataHub>();
	auto equip_hub = std::make_shared<Kernel::EquipmentHub>();
	auto stats_hub = std::make_shared<Kernel::StatisticsHub>();

	// Should not throw — HA discovery should get the data hub too
	BOOST_CHECK_NO_THROW(integration.ConnectHubs(data_hub, equip_hub, stats_hub));
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// MqttIntegration default command handler tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_MqttIntegration_DefaultCommands)

BOOST_AUTO_TEST_CASE(Test_DefaultCommands_RegisteredWhenEnabled)
{
	boost::asio::io_context ioc;
	auto settings = MakeEnabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	auto hub = integration.GetMqttHub();
	BOOST_REQUIRE(hub != nullptr);

	// Should have "status", "device", and "refresh" default commands
	BOOST_CHECK(hub->HasCommand("status"));
	BOOST_CHECK(hub->HasCommand("device"));
	BOOST_CHECK(hub->HasCommand("refresh"));
	BOOST_CHECK_GE(hub->CommandCount(), 3u);
}

BOOST_AUTO_TEST_CASE(Test_DefaultCommands_NotRegisteredWhenDisabled)
{
	boost::asio::io_context ioc;
	auto settings = MakeDisabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	// No hub means no commands
	BOOST_CHECK(integration.GetMqttHub() == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// MqttIntegration access tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_MqttIntegration_Access)

BOOST_AUTO_TEST_CASE(Test_GetMqttHub_WhenEnabled_ReturnsHub)
{
	boost::asio::io_context ioc;
	auto settings = MakeEnabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	auto hub = integration.GetMqttHub();
	BOOST_CHECK(hub != nullptr);
}

BOOST_AUTO_TEST_CASE(Test_GetMqttHub_WhenDisabled_ReturnsNull)
{
	boost::asio::io_context ioc;
	auto settings = MakeDisabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	BOOST_CHECK(integration.GetMqttHub() == nullptr);
}

BOOST_AUTO_TEST_CASE(Test_GetMqttHub_ReturnsSameInstance)
{
	boost::asio::io_context ioc;
	auto settings = MakeEnabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	auto hub1 = integration.GetMqttHub();
	auto hub2 = integration.GetMqttHub();
	BOOST_CHECK_EQUAL(hub1.get(), hub2.get());
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// MqttIntegration factory function tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_MqttIntegration_Factory)

BOOST_AUTO_TEST_CASE(Test_CreateMqttIntegration_WhenEnabled_ReturnsInstance)
{
	boost::asio::io_context ioc;
	auto settings = MakeEnabledSettings();

	auto integration = Mqtt::CreateMqttIntegration(ioc, settings);
	BOOST_CHECK(integration != nullptr);
	BOOST_CHECK(integration->IsEnabled());
}

BOOST_AUTO_TEST_CASE(Test_CreateMqttIntegration_WhenDisabled_ReturnsNull)
{
	boost::asio::io_context ioc;
	auto settings = MakeDisabledSettings();

	auto integration = Mqtt::CreateMqttIntegration(ioc, settings);
	BOOST_CHECK(integration == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// MqttIntegration full lifecycle (no-crash) tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_MqttIntegration_FullLifecycle)

BOOST_AUTO_TEST_CASE(Test_FullLifecycle_StartPollStop_WhenEnabled)
{
	boost::asio::io_context ioc;
	auto settings = MakeEnabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	auto data_hub = std::make_shared<Kernel::DataHub>();
	auto equip_hub = std::make_shared<Kernel::EquipmentHub>();
	auto stats_hub = std::make_shared<Kernel::StatisticsHub>();
	integration.ConnectHubs(data_hub, equip_hub, stats_hub);

	BOOST_CHECK_NO_THROW(integration.Start());
	BOOST_CHECK_NO_THROW(integration.Poll());
	BOOST_CHECK_NO_THROW(integration.Stop());
}

BOOST_AUTO_TEST_CASE(Test_FullLifecycle_StartPollStop_WhenDisabled)
{
	boost::asio::io_context ioc;
	auto settings = MakeDisabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	BOOST_CHECK_NO_THROW(integration.Start());
	BOOST_CHECK_NO_THROW(integration.Poll());
	BOOST_CHECK_NO_THROW(integration.Stop());
}

BOOST_AUTO_TEST_CASE(Test_FullLifecycle_WithHomeAssistant)
{
	boost::asio::io_context ioc;
	auto settings = MakeHaEnabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	auto data_hub = std::make_shared<Kernel::DataHub>();
	auto equip_hub = std::make_shared<Kernel::EquipmentHub>();
	auto stats_hub = std::make_shared<Kernel::StatisticsHub>();
	integration.ConnectHubs(data_hub, equip_hub, stats_hub);

	BOOST_CHECK_NO_THROW(integration.Start());
	BOOST_CHECK_NO_THROW(integration.Poll());
	BOOST_CHECK_NO_THROW(integration.Stop());
}

BOOST_AUTO_TEST_SUITE_END()
