#include <boost/test/unit_test.hpp>

#include "support/unit_test_onetouchdevice.h"
#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate;

BOOST_FIXTURE_TEST_SUITE(PageProcessor_EquipmentOnOff_TestSuite, Test::OneTouchDevice)

BOOST_AUTO_TEST_CASE(TestAuxillariesHeatersPumpsAreRegistered)
{
	auto check_for_device = [](const auto& label, const auto& state, const auto& device_vec) -> bool
	{
		for (const auto& device : device_vec)
		{
			if (nullptr == device)
			{
				// Not a valid device...ignore
			}
			else if (label != *(device->AuxillaryTraits[Kernel::AuxillaryTraitsTypes::LabelTrait{}]))
			{
				// Label doesn't match...ignore.
			}
			else 
			{
				switch(*(device->AuxillaryTraits[Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{}]))
				{
				case Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Auxillary:
					return (*(device->AuxillaryTraits[Kernel::AuxillaryTraitsTypes::AuxillaryStatusTrait{}]) == static_cast<AuxillaryStatuses>(state));

				case Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Chlorinator:
					return (*(device->AuxillaryTraits[Kernel::AuxillaryTraitsTypes::ChlorinatorStatusTrait{}]) == static_cast<ChlorinatorStatuses>(state));

				case Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Heater:
					return (*(device->AuxillaryTraits[Kernel::AuxillaryTraitsTypes::HeaterStatusTrait{}]) == static_cast<HeaterStatuses>(state));

				case Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Pump:
					return (*(device->AuxillaryTraits[Kernel::AuxillaryTraitsTypes::PumpStatusTrait{}]) == static_cast<PumpStatuses>(state));

				case Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Cleaner:
					[[fallthrough]];
				case Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Spillover:
					[[fallthrough]];
				case Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Sprinkler:
					[[fallthrough]];
				case Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Unknown:
					[[fallthrough]];
				default:
					break;
				}
			}
		}

		return false;
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

	BOOST_CHECK(check_for_device("Aux1", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux2", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux3", Kernel::AuxillaryStatuses::On, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux4", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux5", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux6", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux7", Kernel::AuxillaryStatuses::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux8", Kernel::AuxillaryStatuses::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B1", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B2", Kernel::AuxillaryStatuses::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B3", Kernel::AuxillaryStatuses::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B4", Kernel::AuxillaryStatuses::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B5", Kernel::AuxillaryStatuses::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B6", Kernel::AuxillaryStatuses::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B7", Kernel::AuxillaryStatuses::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux B8", Kernel::AuxillaryStatuses::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Extra Aux", Kernel::AuxillaryStatuses::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Pool Heat", Kernel::HeaterStatuses::Enabled, data_hub.Heaters()));
	BOOST_CHECK(check_for_device("Spa Heat", Kernel::HeaterStatuses::Off, data_hub.Heaters()));
	BOOST_CHECK(!check_for_device("Solar Heat", Kernel::HeaterStatuses::Unknown, data_hub.Heaters()));
	BOOST_CHECK(!check_for_device("Heat Pump", Kernel::HeaterStatuses::Unknown, data_hub.Heaters()));
	BOOST_CHECK(check_for_device("Pool Pump", Kernel::PumpStatuses::Unknown, data_hub.Pumps()));
	BOOST_CHECK(check_for_device("Spa Pump", Kernel::PumpStatuses::Running, data_hub.Pumps()));
	BOOST_CHECK(!check_for_device("Filter Pump", Kernel::PumpStatuses::Unknown, data_hub.Pumps()));
	BOOST_CHECK(!check_for_device("Heat Pump", Kernel::PumpStatuses::Unknown, data_hub.Pumps())); // Ensure that the "pump" didn't go to the wrong place!

	// Check for mis-parsing of the all off command.
	BOOST_CHECK(!check_for_device("All Off", Kernel::AuxillaryStatuses::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("All Off", Kernel::HeaterStatuses::Unknown, data_hub.Heaters()));
	BOOST_CHECK(!check_for_device("All Off", Kernel::PumpStatuses::Unknown, data_hub.Pumps()));

	EquipmentOnOff_Page2();

	BOOST_CHECK_EQUAL(15, data_hub.Auxillaries().size());
	BOOST_CHECK_EQUAL(2, data_hub.Heaters().size());
	BOOST_CHECK_EQUAL(2, data_hub.Pumps().size());

	BOOST_CHECK(check_for_device("Aux1", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux2", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux3", Kernel::AuxillaryStatuses::On, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux4", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux5", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux6", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux7", Kernel::AuxillaryStatuses::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux8", Kernel::AuxillaryStatuses::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B1", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B2", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B3", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B4", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B5", Kernel::AuxillaryStatuses::On, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B6", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B7", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B8", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Extra Aux", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Pool Heat", Kernel::HeaterStatuses::Enabled, data_hub.Heaters()));
	BOOST_CHECK(check_for_device("Spa Heat", Kernel::HeaterStatuses::Off, data_hub.Heaters()));
	BOOST_CHECK(!check_for_device("Solar Heat", Kernel::HeaterStatuses::Unknown, data_hub.Heaters()));
	BOOST_CHECK(!check_for_device("Heat Pump", Kernel::HeaterStatuses::Unknown, data_hub.Heaters()));
	BOOST_CHECK(check_for_device("Pool Pump", Kernel::PumpStatuses::Unknown, data_hub.Pumps()));
	BOOST_CHECK(check_for_device("Spa Pump", Kernel::PumpStatuses::Running, data_hub.Pumps()));
	BOOST_CHECK(!check_for_device("Filter Pump", Kernel::PumpStatuses::Unknown, data_hub.Pumps()));
	BOOST_CHECK(!check_for_device("Heat Pump", Kernel::PumpStatuses::Unknown, data_hub.Pumps())); // Ensure that the "pump" didn't go to the wrong place!

	// Check for mis-parsing of the all off command.
	BOOST_CHECK(!check_for_device("All Off", Kernel::AuxillaryStatuses::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("All Off", Kernel::HeaterStatuses::Unknown, data_hub.Heaters()));
	BOOST_CHECK(!check_for_device("All Off", Kernel::PumpStatuses::Unknown, data_hub.Pumps()));

	EquipmentOnOff_Page3();

	BOOST_CHECK_EQUAL(15, data_hub.Auxillaries().size());
	BOOST_CHECK_EQUAL(2, data_hub.Heaters().size());
	BOOST_CHECK_EQUAL(2, data_hub.Pumps().size());

	BOOST_CHECK(check_for_device("Aux1", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux2", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux3", Kernel::AuxillaryStatuses::On, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux4", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux5", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux6", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux7", Kernel::AuxillaryStatuses::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("Aux8", Kernel::AuxillaryStatuses::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B1", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B2", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B3", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B4", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B5", Kernel::AuxillaryStatuses::On, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B6", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B7", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Aux B8", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Extra Aux", Kernel::AuxillaryStatuses::Off, data_hub.Auxillaries()));
	BOOST_CHECK(check_for_device("Pool Heat", Kernel::HeaterStatuses::Enabled, data_hub.Heaters()));
	BOOST_CHECK(check_for_device("Spa Heat", Kernel::HeaterStatuses::Off, data_hub.Heaters()));
	BOOST_CHECK(!check_for_device("Solar Heat", Kernel::HeaterStatuses::Unknown, data_hub.Heaters()));
	BOOST_CHECK(!check_for_device("Heat Pump", Kernel::HeaterStatuses::Unknown, data_hub.Heaters()));
	BOOST_CHECK(check_for_device("Pool Pump", Kernel::PumpStatuses::Unknown, data_hub.Pumps()));
	BOOST_CHECK(check_for_device("Spa Pump", Kernel::PumpStatuses::Running, data_hub.Pumps()));
	BOOST_CHECK(!check_for_device("Filter Pump", Kernel::PumpStatuses::Unknown, data_hub.Pumps()));
	BOOST_CHECK(!check_for_device("Heat Pump", Kernel::PumpStatuses::Unknown, data_hub.Pumps())); // Ensure that the "pump" didn't go to the wrong place!

	// Check for mis-parsing of the all off command.
	BOOST_CHECK(!check_for_device("All Off", Kernel::AuxillaryStatuses::Unknown, data_hub.Auxillaries()));
	BOOST_CHECK(!check_for_device("All Off", Kernel::HeaterStatuses::Unknown, data_hub.Heaters()));
	BOOST_CHECK(!check_for_device("All Off", Kernel::PumpStatuses::Unknown, data_hub.Pumps()));
}

BOOST_AUTO_TEST_SUITE_END()
