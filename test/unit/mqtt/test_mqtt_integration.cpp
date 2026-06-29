#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

#include "mqtt/mqtt_integration.h"
#include "mqtt/mqtt_hub.h"
#include "mqtt/mqtt_client.h"
#include "interfaces/icommanddispatcher.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/hub_locator.h"
#include "kernel/statistics_hub.h"
#include "kernel/body_of_water_ids.h"
#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
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

	// Should have "status" and "refresh" default commands before hubs are connected.
	BOOST_CHECK(hub->HasCommand("status"));
	BOOST_CHECK(hub->HasCommand("refresh"));
	BOOST_CHECK_GE(hub->CommandCount(), 2u);

	// The "device" command is NOT registered until the command dispatcher is available
	// (it used to be registered here as a dead placeholder that only echoed "acknowledged").
	BOOST_CHECK(!hub->HasCommand("device"));
}

BOOST_AUTO_TEST_CASE(Test_DeviceCommand_RegisteredAfterHubsConnected)
{
	boost::asio::io_context ioc;
	auto settings = MakeEnabledSettings();
	Mqtt::MqttIntegration integration(ioc, settings);

	auto hub = integration.GetMqttHub();
	BOOST_REQUIRE(hub != nullptr);

	// Before connecting hubs, the "device" command handler should not exist.
	BOOST_CHECK(!hub->HasCommand("device"));

	auto data_hub = std::make_shared<Kernel::DataHub>();
	auto equip_hub = std::make_shared<Kernel::EquipmentHub>();
	auto stats_hub = std::make_shared<Kernel::StatisticsHub>();
	integration.ConnectHubs(data_hub, equip_hub, stats_hub);

	// After connecting hubs, the real "device" and "setpoint" handlers are registered.
	BOOST_CHECK(hub->HasCommand("device"));
	BOOST_CHECK(hub->HasCommand("setpoint"));
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

//=============================================================================
// MqttIntegration dynamic heater-command tests
//
// Heaters are actuated through a dedicated heater/{slug} -> SetHeaterMode(body,
// on/off) route registered by RegisterDynamicDeviceCommands() (fired from the
// hub's OnDevicesPublished signal). These tests drive that path end to end:
// build a DataHub heater, connect a recording dispatcher via the locator, fire
// the signal, then push a command through the public HandleMessage() seam.
//=============================================================================

namespace
{
	namespace Traits = Kernel::AuxillaryTraitsTypes;

	// Recording dispatcher: captures heater commands so a test can prove the
	// heater/{slug} route maps the ON/OFF payload to the right body + enable flag.
	class RecordingHeaterDispatcher : public Interfaces::ICommandDispatcher
	{
	public:
		std::vector<std::pair<Kernel::BodyOfWaterIds, bool>> heater_calls;

		CommandResult ToggleByUuid(const boost::uuids::uuid&) override { return CommandResult::Success; }
		CommandResult ToggleByLabel(const std::string&) override { return CommandResult::Success; }
		CommandResult CommandByUuid(const boost::uuids::uuid&, DeviceAction) override { return CommandResult::Success; }
		CommandResult CommandByLabel(const std::string&, DeviceAction) override { return CommandResult::Success; }
		CommandResult SetPoolSetpoint(std::uint8_t) override { return CommandResult::Success; }
		CommandResult SetSpaSetpoint(std::uint8_t) override { return CommandResult::Success; }
		CommandResult SetChlorinatorPercentage(std::uint8_t) override { return CommandResult::Success; }
		CommandResult SetChlorinatorBoost(bool) override { return CommandResult::Success; }
		CommandResult SetCirculationMode(Kernel::CirculationModes) override { return CommandResult::Success; }
		CommandResult SetHeaterMode(Kernel::BodyOfWaterIds body, bool enable) override { heater_calls.emplace_back(body, enable); return CommandResult::Success; }
		CommandResult SelectIAQPageButton(std::uint8_t) override { return CommandResult::Success; }
	};

	struct HeaterCommandFixture
	{
		boost::asio::io_context ioc;
		std::shared_ptr<RecordingHeaterDispatcher> dispatcher{ std::make_shared<RecordingHeaterDispatcher>() };
		std::shared_ptr<Kernel::DataHub> data_hub{ std::make_shared<Kernel::DataHub>() };
		std::shared_ptr<Kernel::EquipmentHub> equip_hub{ std::make_shared<Kernel::EquipmentHub>() };
		std::shared_ptr<Kernel::StatisticsHub> stats_hub{ std::make_shared<Kernel::StatisticsHub>() };
		Mqtt::MqttIntegration integration;

		HeaterCommandFixture()
			: integration(ioc, MakeHaEnabledSettings())
		{
		}

		~HeaterCommandFixture()
		{
			integration.Stop();
		}

		// Add a heater AuxillaryDevice to the DataHub so DataHub::Heaters() returns it.
		// with_body=false omits the BodyOfWaterTrait to exercise the skip path.
		void AddHeater(const std::string& label, Kernel::BodyOfWaterIds body, bool with_body = true)
		{
			auto heater = std::make_shared<Kernel::AuxillaryDevice>();
			heater->AuxillaryTraits.Set(Traits::AuxillaryTypeTrait{}, Traits::AuxillaryTypes::Heater);
			heater->AuxillaryTraits.Set(Traits::LabelTrait{}, label);
			if (with_body)
			{
				heater->AuxillaryTraits.Set(Traits::BodyOfWaterTrait{}, body);
			}
			data_hub->Devices.Add(heater);
		}

		// Connect hubs + dispatcher via the locator (the only overload that wires the
		// command dispatcher), start (connects OnDevicesPublished -> dynamic command
		// registration, HA enabled), then fire the signal to run the registration.
		Mqtt::MqttHub& ConnectAndPublish()
		{
			Kernel::HubLocator locator;
			locator.Register(data_hub).Register(equip_hub).Register(stats_hub)
				.Register(std::static_pointer_cast<Interfaces::ICommandDispatcher>(dispatcher));
			integration.ConnectHubs(locator);
			integration.Start();

			auto hub = integration.GetMqttHub();
			BOOST_REQUIRE(hub != nullptr);
			hub->OnDevicesPublished();
			return *hub;
		}
	};
}

BOOST_FIXTURE_TEST_SUITE(TestSuite_MqttIntegration_HeaterCommands, HeaterCommandFixture)

BOOST_AUTO_TEST_CASE(Test_HeaterCommand_RegisteredForLabelledHeaterWithBody)
{
	AddHeater("Pool Heater", Kernel::BodyOfWaterIds::Pool);
	auto& hub = ConnectAndPublish();

	BOOST_CHECK(hub.HasCommand("heater/pool_heater"));
}

BOOST_AUTO_TEST_CASE(Test_HeaterCommand_On_DispatchesEnable)
{
	AddHeater("Pool Heater", Kernel::BodyOfWaterIds::Pool);
	auto& hub = ConnectAndPublish();
	BOOST_REQUIRE(hub.HasCommand("heater/pool_heater"));

	// Inject the inbound command via the client's public OnMessageReceived signal,
	// which the hub subscribes to and routes through to the registered handler.
	hub.GetMqttClient()->OnMessageReceived(hub.CommandTopic("heater/pool_heater"), "ON");

	BOOST_REQUIRE_EQUAL(dispatcher->heater_calls.size(), 1u);
	BOOST_CHECK(dispatcher->heater_calls[0].first == Kernel::BodyOfWaterIds::Pool);
	BOOST_CHECK_EQUAL(dispatcher->heater_calls[0].second, true);
}

BOOST_AUTO_TEST_CASE(Test_HeaterCommand_Off_DispatchesDisable)
{
	AddHeater("Spa Heater", Kernel::BodyOfWaterIds::Spa);
	auto& hub = ConnectAndPublish();
	BOOST_REQUIRE(hub.HasCommand("heater/spa_heater"));

	hub.GetMqttClient()->OnMessageReceived(hub.CommandTopic("heater/spa_heater"), "OFF");

	BOOST_REQUIRE_EQUAL(dispatcher->heater_calls.size(), 1u);
	BOOST_CHECK(dispatcher->heater_calls[0].first == Kernel::BodyOfWaterIds::Spa);
	BOOST_CHECK_EQUAL(dispatcher->heater_calls[0].second, false);
}

BOOST_AUTO_TEST_CASE(Test_HeaterCommand_UnknownPayload_NotDispatched)
{
	AddHeater("Pool Heater", Kernel::BodyOfWaterIds::Pool);
	auto& hub = ConnectAndPublish();
	BOOST_REQUIRE(hub.HasCommand("heater/pool_heater"));

	hub.GetMqttClient()->OnMessageReceived(hub.CommandTopic("heater/pool_heater"), "MAYBE");

	BOOST_CHECK(dispatcher->heater_calls.empty());
}

BOOST_AUTO_TEST_CASE(Test_HeaterCommand_NotRegistered_WhenBodyTraitMissing)
{
	AddHeater("Orphan Heater", Kernel::BodyOfWaterIds::Pool, /*with_body=*/false);
	auto& hub = ConnectAndPublish();

	BOOST_CHECK(!hub.HasCommand("heater/orphan_heater"));
}

BOOST_AUTO_TEST_SUITE_END()
