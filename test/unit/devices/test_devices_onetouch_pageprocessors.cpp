#include <boost/test/unit_test.hpp>

#include "support/unit_test_onetouchdevice.h"
#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate;

BOOST_FIXTURE_TEST_SUITE(PageProcessor_EquipmentOnOff_TestSuite, Test::OneTouchDevice)

BOOST_AUTO_TEST_CASE(TestAuxillariesHeatersPumpsAreRegistered)
{
	auto check_for_device = [](const auto& label, const auto& state, const auto& device_vec) -> bool
	{
		bool was_found = false;

		for (const auto& device : device_vec)
		{
			if (nullptr == device)
			{
				// Not a valid device...ignore
			}
			else if (label != device->Label())
			{
				// Label doesn't match...ignore.
			}
			else if (state != device->State())
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

	auto& config = Config();

	BOOST_REQUIRE_EQUAL(config.PoolConfiguration, Config::PoolConfigurations::Unknown);
	BOOST_REQUIRE_EQUAL(config.SystemBoard, Config::SystemBoards::Unknown);

	InitialiseOneTouchDevice();

	BOOST_REQUIRE_NE(config.PoolConfiguration, Config::PoolConfigurations::Unknown);
	BOOST_REQUIRE_NE(config.SystemBoard, Config::SystemBoards::Unknown);

	EquipmentOnOff_Page1();

	BOOST_CHECK_EQUAL(7, config.Auxillaries().size());
	BOOST_CHECK_EQUAL(2, config.Heaters().size());
	BOOST_CHECK_EQUAL(2, config.Pumps().size());

	BOOST_CHECK(check_for_device("Aux1", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux2", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux3", Config::AuxillaryStates::On, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux4", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux5", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux6", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux7", Config::AuxillaryStates::Unknown, config.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux8", Config::AuxillaryStates::Unknown, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B1", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B2", Config::AuxillaryStates::Unknown, config.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B3", Config::AuxillaryStates::Unknown, config.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B4", Config::AuxillaryStates::Unknown, config.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B5", Config::AuxillaryStates::Unknown, config.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B6", Config::AuxillaryStates::Unknown, config.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B7", Config::AuxillaryStates::Unknown, config.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B8", Config::AuxillaryStates::Unknown, config.Auxillaries()));
	BOOST_CHECK(!check_for_device("Extra Aux", Config::AuxillaryStates::Unknown, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Pool Heat", Config::HeaterStatus::Enabled, config.Heaters()));
	BOOST_CHECK(check_for_device("Spa Heat", Config::HeaterStatus::Off, config.Heaters()));
	BOOST_CHECK(!check_for_device("Solar Heat", Config::HeaterStatus::Unknown, config.Heaters()));
	BOOST_CHECK(!check_for_device("Heat Pump", Config::HeaterStatus::Unknown, config.Heaters()));
	BOOST_CHECK(check_for_device("Pool Pump", Config::PumpStatus::Unknown, config.Pumps()));
	BOOST_CHECK(check_for_device("Spa Pump", Config::PumpStatus::Running, config.Pumps()));
	BOOST_CHECK(!check_for_device("Filter Pump", Config::PumpStatus::Unknown, config.Pumps()));
	BOOST_CHECK(!check_for_device("Heat Pump", Config::PumpStatus::Unknown, config.Pumps())); // Ensure that the "pump" didn't go to the wrong place!

	// Check for mis-parsing of the all off command.
	BOOST_CHECK(!check_for_device("All Off", Config::AuxillaryStates::Unknown, config.Auxillaries()));
	BOOST_CHECK(!check_for_device("All Off", Config::HeaterStatus::Unknown, config.Heaters()));
	BOOST_CHECK(!check_for_device("All Off", Config::PumpStatus::Unknown, config.Pumps()));

	EquipmentOnOff_Page2();

	BOOST_CHECK_EQUAL(15, config.Auxillaries().size());
	BOOST_CHECK_EQUAL(2, config.Heaters().size());
	BOOST_CHECK_EQUAL(2, config.Pumps().size());

	BOOST_CHECK(check_for_device("Aux1", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux2", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux3", Config::AuxillaryStates::On, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux4", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux5", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux6", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux7", Config::AuxillaryStates::Unknown, config.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux8", Config::AuxillaryStates::Unknown, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B1", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B2", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B3", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B4", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B5", Config::AuxillaryStates::On, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B6", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B7", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B8", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Extra Aux", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Pool Heat", Config::HeaterStatus::Enabled, config.Heaters()));
	BOOST_CHECK(check_for_device("Spa Heat", Config::HeaterStatus::Off, config.Heaters()));
	BOOST_CHECK(!check_for_device("Solar Heat", Config::HeaterStatus::Unknown, config.Heaters()));
	BOOST_CHECK(!check_for_device("Heat Pump", Config::HeaterStatus::Unknown, config.Heaters()));
	BOOST_CHECK(check_for_device("Pool Pump", Config::PumpStatus::Unknown, config.Pumps()));
	BOOST_CHECK(check_for_device("Spa Pump", Config::PumpStatus::Running, config.Pumps()));
	BOOST_CHECK(!check_for_device("Filter Pump", Config::PumpStatus::Unknown, config.Pumps()));
	BOOST_CHECK(!check_for_device("Heat Pump", Config::PumpStatus::Unknown, config.Pumps())); // Ensure that the "pump" didn't go to the wrong place!

	// Check for mis-parsing of the all off command.
	BOOST_CHECK(!check_for_device("All Off", Config::AuxillaryStates::Unknown, config.Auxillaries()));
	BOOST_CHECK(!check_for_device("All Off", Config::HeaterStatus::Unknown, config.Heaters()));
	BOOST_CHECK(!check_for_device("All Off", Config::PumpStatus::Unknown, config.Pumps()));

	EquipmentOnOff_Page3();

	BOOST_CHECK_EQUAL(15, config.Auxillaries().size());
	BOOST_CHECK_EQUAL(2, config.Heaters().size());
	BOOST_CHECK_EQUAL(2, config.Pumps().size());

	BOOST_CHECK(check_for_device("Aux1", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux2", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux3", Config::AuxillaryStates::On, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux4", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux5", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux6", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux7", Config::AuxillaryStates::Unknown, config.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux8", Config::AuxillaryStates::Unknown, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B1", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B2", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B3", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B4", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B5", Config::AuxillaryStates::On, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B6", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B7", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B8", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Extra Aux", Config::AuxillaryStates::Off, config.Auxillaries()));
	BOOST_CHECK(check_for_device("Pool Heat", Config::HeaterStatus::Enabled, config.Heaters()));
	BOOST_CHECK(check_for_device("Spa Heat", Config::HeaterStatus::Off, config.Heaters()));
	BOOST_CHECK(!check_for_device("Solar Heat", Config::HeaterStatus::Unknown, config.Heaters()));
	BOOST_CHECK(!check_for_device("Heat Pump", Config::HeaterStatus::Unknown, config.Heaters()));
	BOOST_CHECK(check_for_device("Pool Pump", Config::PumpStatus::Unknown, config.Pumps()));
	BOOST_CHECK(check_for_device("Spa Pump", Config::PumpStatus::Running, config.Pumps()));
	BOOST_CHECK(!check_for_device("Filter Pump", Config::PumpStatus::Unknown, config.Pumps()));
	BOOST_CHECK(!check_for_device("Heat Pump", Config::PumpStatus::Unknown, config.Pumps())); // Ensure that the "pump" didn't go to the wrong place!

	// Check for mis-parsing of the all off command.
	BOOST_CHECK(!check_for_device("All Off", Config::AuxillaryStates::Unknown, config.Auxillaries()));
	BOOST_CHECK(!check_for_device("All Off", Config::HeaterStatus::Unknown, config.Heaters()));
	BOOST_CHECK(!check_for_device("All Off", Config::PumpStatus::Unknown, config.Pumps()));
}

BOOST_AUTO_TEST_SUITE_END()
