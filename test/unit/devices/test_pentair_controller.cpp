#include <cstdint>
#include <memory>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "kernel/data_hub.h"
#include "kernel/temperature.h"

#include "pentair/devices/pentair_controller_device.h"
#include "pentair/devices/pentair_device_id.h"
#include "pentair/equipment/pentair_equipment.h"
#include "pentair/messages/controller/pentair_controller_message_status.h"
#include "pentair/messages/pentair_message_ids.h"

#include "support/unit_test_mockreplayharness.h"
#include "support/unit_test_protocolmessagebuilder.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Pentair;

//=============================================================================
// Pentair B4: IntelliCenter / EasyTouch controller support.
//=============================================================================

namespace
{
	constexpr uint8_t CONTROLLER = 0x10;
	constexpr uint8_t BROADCAST = 0x0F;
	const uint8_t CMD_STATUS = static_cast<uint8_t>(Messages::PentairMessageIds::Controller_Status);

	using CtrlMsg = Messages::PentairControllerMessage_Status;
}

BOOST_AUTO_TEST_SUITE(TestSuite_PentairController)

//-----------------------------------------------------------------------------
// Pool mode: water temp routes to pool temp, air temp set, heater state decoded.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Replay_ControllerStatus_PoolMode_SurfacesTemps)
{
	Test::MockReplayHarness harness;

	auto device_id = std::make_shared<Pentair::Devices::PentairDeviceId>(CONTROLLER);
	auto& device = harness.AddDevice<Pentair::Devices::PentairControllerDevice>(device_id);

	// 14:30, pool circuit on, water 82F, air 75F, pool heater on.
	std::vector<uint8_t> data = {
		14, 30,
		CtrlMsg::CIRCUIT_POOL,
		82, 75,
		CtrlMsg::HEATER_POOL
	};

	auto frame = Test::PentairMessageBuilder::CreateValidChecksummedFrame(CONTROLLER, BROADCAST, CMD_STATUS, data);
	harness.Replay(frame);

	BOOST_TEST(harness.StatisticsHub()->MessageErrors.ChecksumFailures == 0u);

	// Units switched to Fahrenheit by the controller.
	BOOST_CHECK(harness.DataHub()->SystemTemperatureUnits() == Kernel::TemperatureUnits::Fahrenheit);

	// Pool temp populated (spa circuit off -> water reading is the pool).
	auto pool_temp = harness.DataHub()->PoolTemp();
	BOOST_REQUIRE(pool_temp.has_value());
	BOOST_CHECK_CLOSE(pool_temp.value().InFahrenheit().value(), 82.0, 0.01);

	auto air_temp = harness.DataHub()->AirTemp();
	BOOST_REQUIRE(air_temp.has_value());
	BOOST_CHECK_CLOSE(air_temp.value().InFahrenheit().value(), 75.0, 0.01);

	// Heater decode.
	BOOST_CHECK(device.PoolHeaterOn());
	BOOST_CHECK(!device.SpaHeaterOn());

	// Circulation mode reflects pool.
	BOOST_CHECK(harness.DataHub()->CirculationMode == Kernel::CirculationModes::Pool);
}

//-----------------------------------------------------------------------------
// Spa mode: water temp routes to spa temp; circulation mode is Spa.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Replay_ControllerStatus_SpaMode_RoutesWaterToSpa)
{
	Test::MockReplayHarness harness;

	auto device_id = std::make_shared<Pentair::Devices::PentairDeviceId>(CONTROLLER);
	auto& device = harness.AddDevice<Pentair::Devices::PentairControllerDevice>(device_id);

	// Spa circuit on, water 102F, air 70F, spa heater on.
	std::vector<uint8_t> data = {
		9, 15,
		CtrlMsg::CIRCUIT_SPA,
		102, 70,
		CtrlMsg::HEATER_SPA
	};

	auto frame = Test::PentairMessageBuilder::CreateValidChecksummedFrame(CONTROLLER, BROADCAST, CMD_STATUS, data);
	harness.Replay(frame);

	auto spa_temp = harness.DataHub()->SpaTemp();
	BOOST_REQUIRE(spa_temp.has_value());
	BOOST_CHECK_CLOSE(spa_temp.value().InFahrenheit().value(), 102.0, 0.01);

	BOOST_CHECK(device.SpaHeaterOn());
	BOOST_CHECK(!device.PoolHeaterOn());
	BOOST_CHECK(harness.DataHub()->CirculationMode == Kernel::CirculationModes::Spa);
}

//-----------------------------------------------------------------------------
// Equipment discovery: a controller status frame auto-registers a controller
// device in the EquipmentHub.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Discovery_ControllerStatus_AddsControllerDevice)
{
	Test::MockReplayHarness harness;

	// No device added up-front; PentairEquipment should discover it.
	auto equipment = std::make_unique<Pentair::Equipment::PentairEquipment>(harness.HubLocatorRef());
	harness.EquipmentHub()->AddEquipment(std::move(equipment));

	std::vector<uint8_t> data = { 12, 0, CtrlMsg::CIRCUIT_POOL, 80, 72, 0 };
	auto frame = Test::PentairMessageBuilder::CreateValidChecksummedFrame(CONTROLLER, BROADCAST, CMD_STATUS, data);
	harness.Replay(frame);

	Pentair::Devices::PentairDeviceId controller_id(CONTROLLER);
	BOOST_CHECK(harness.EquipmentHub()->DeviceExists(controller_id));
}

BOOST_AUTO_TEST_SUITE_END()
