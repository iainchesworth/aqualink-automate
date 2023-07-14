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

	auto& data_hub = DataHub();

	BOOST_REQUIRE_EQUAL(data_hub.PoolConfiguration, Kernel::PoolConfigurations::Unknown);
	BOOST_REQUIRE_EQUAL(data_hub.SystemBoard, Kernel::SystemBoards::Unknown);

	InitialiseOneTouchDevice();

	BOOST_REQUIRE_NE(data_hub.PoolConfiguration, Kernel::PoolConfigurations::Unknown);
	BOOST_REQUIRE_NE(data_hub.SystemBoard, Kernel::SystemBoards::Unknown);

	EquipmentOnOff_Page1();

	BOOST_CHECK_EQUAL(7, data_hub.Auxillaries().size());
	BOOST_CHECK_EQUAL(2, data_hub.Heaters().size());
	BOOST_CHECK_EQUAL(2, data_hub.Pumps().size());

	BOOST_CHECK(check_for_device("Aux1", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux2", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux3", Kernel::AuxillaryStates::On, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux4", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux5", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux6", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux7", Kernel::AuxillaryStates::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux8", Kernel::AuxillaryStates::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B1", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B2", Kernel::AuxillaryStates::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B3", Kernel::AuxillaryStates::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B4", Kernel::AuxillaryStates::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B5", Kernel::AuxillaryStates::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B6", Kernel::AuxillaryStates::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B7", Kernel::AuxillaryStates::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B8", Kernel::AuxillaryStates::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Extra Aux", Kernel::AuxillaryStates::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Pool Heat", Kernel::HeaterStatus::Enabled, data_hub.Heaters()));
	BOOST_CHECK(check_for_device("Spa Heat", Kernel::HeaterStatus::Off, data_hub.Heaters()));
	BOOST_CHECK(!check_for_device("Solar Heat", Kernel::HeaterStatus::Unknown, data_hub.Heaters()));
	BOOST_CHECK(!check_for_device("Heat Pump", Kernel::HeaterStatus::Unknown, data_hub.Heaters()));
	BOOST_CHECK(check_for_device("Pool Pump", Kernel::PumpStatus::Unknown, data_hub.Pumps()));
	BOOST_CHECK(check_for_device("Spa Pump", Kernel::PumpStatus::Running, data_hub.Pumps()));
	BOOST_CHECK(!check_for_device("Filter Pump", Kernel::PumpStatus::Unknown, data_hub.Pumps()));
	BOOST_CHECK(!check_for_device("Heat Pump", Kernel::PumpStatus::Unknown, data_hub.Pumps())); // Ensure that the "pump" didn't go to the wrong place!

	// Check for mis-parsing of the all off command.
	BOOST_CHECK(!check_for_device("All Off", Kernel::AuxillaryStates::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("All Off", Kernel::HeaterStatus::Unknown, data_hub.Heaters()));
	BOOST_CHECK(!check_for_device("All Off", Kernel::PumpStatus::Unknown, data_hub.Pumps()));

	EquipmentOnOff_Page2();

	BOOST_CHECK_EQUAL(15, data_hub.Auxillaries().size());
	BOOST_CHECK_EQUAL(2, data_hub.Heaters().size());
	BOOST_CHECK_EQUAL(2, data_hub.Pumps().size());

	BOOST_CHECK(check_for_device("Aux1", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux2", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux3", Kernel::AuxillaryStates::On, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux4", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux5", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux6", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux7", Kernel::AuxillaryStates::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux8", Kernel::AuxillaryStates::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B1", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B2", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B3", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B4", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B5", Kernel::AuxillaryStates::On, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B6", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B7", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B8", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Extra Aux", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Pool Heat", Kernel::HeaterStatus::Enabled, data_hub.Heaters()));
	BOOST_CHECK(check_for_device("Spa Heat", Kernel::HeaterStatus::Off, data_hub.Heaters()));
	BOOST_CHECK(!check_for_device("Solar Heat", Kernel::HeaterStatus::Unknown, data_hub.Heaters()));
	BOOST_CHECK(!check_for_device("Heat Pump", Kernel::HeaterStatus::Unknown, data_hub.Heaters()));
	BOOST_CHECK(check_for_device("Pool Pump", Kernel::PumpStatus::Unknown, data_hub.Pumps()));
	BOOST_CHECK(check_for_device("Spa Pump", Kernel::PumpStatus::Running, data_hub.Pumps()));
	BOOST_CHECK(!check_for_device("Filter Pump", Kernel::PumpStatus::Unknown, data_hub.Pumps()));
	BOOST_CHECK(!check_for_device("Heat Pump", Kernel::PumpStatus::Unknown, data_hub.Pumps())); // Ensure that the "pump" didn't go to the wrong place!

	// Check for mis-parsing of the all off command.
	BOOST_CHECK(!check_for_device("All Off", Kernel::AuxillaryStates::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("All Off", Kernel::HeaterStatus::Unknown, data_hub.Heaters()));
	BOOST_CHECK(!check_for_device("All Off", Kernel::PumpStatus::Unknown, data_hub.Pumps()));

	EquipmentOnOff_Page3();

	BOOST_CHECK_EQUAL(15, data_hub.Auxillaries().size());
	BOOST_CHECK_EQUAL(2, data_hub.Heaters().size());
	BOOST_CHECK_EQUAL(2, data_hub.Pumps().size());

	BOOST_CHECK(check_for_device("Aux1", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux2", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux3", Kernel::AuxillaryStates::On, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux4", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux5", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux6", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux7", Kernel::AuxillaryStates::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux8", Kernel::AuxillaryStates::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B1", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B2", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B3", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B4", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B5", Kernel::AuxillaryStates::On, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B6", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B7", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B8", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Extra Aux", Kernel::AuxillaryStates::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Pool Heat", Kernel::HeaterStatus::Enabled, data_hub.Heaters()));
	BOOST_CHECK(check_for_device("Spa Heat", Kernel::HeaterStatus::Off, data_hub.Heaters()));
	BOOST_CHECK(!check_for_device("Solar Heat", Kernel::HeaterStatus::Unknown, data_hub.Heaters()));
	BOOST_CHECK(!check_for_device("Heat Pump", Kernel::HeaterStatus::Unknown, data_hub.Heaters()));
	BOOST_CHECK(check_for_device("Pool Pump", Kernel::PumpStatus::Unknown, data_hub.Pumps()));
	BOOST_CHECK(check_for_device("Spa Pump", Kernel::PumpStatus::Running, data_hub.Pumps()));
	BOOST_CHECK(!check_for_device("Filter Pump", Kernel::PumpStatus::Unknown, data_hub.Pumps()));
	BOOST_CHECK(!check_for_device("Heat Pump", Kernel::PumpStatus::Unknown, data_hub.Pumps())); // Ensure that the "pump" didn't go to the wrong place!

	// Check for mis-parsing of the all off command.
	BOOST_CHECK(!check_for_device("All Off", Kernel::AuxillaryStates::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("All Off", Kernel::HeaterStatus::Unknown, data_hub.Heaters()));
	BOOST_CHECK(!check_for_device("All Off", Kernel::PumpStatus::Unknown, data_hub.Pumps()));
}

BOOST_AUTO_TEST_SUITE_END()
