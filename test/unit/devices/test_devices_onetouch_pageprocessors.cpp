#include <boost/test/unit_test.hpp>

#include "kernel/auxillary_devices/chlorinator_status.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"

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
			// Skip null devices and non-matching labels
			if (nullptr == device)
			{
				continue;
			}
			if (label != *(device->AuxillaryTraits[Kernel::AuxillaryTraitsTypes::LabelTrait{}]))
			{
				continue;
			}
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

//=============================================================================
// OneTouch "EQUIPMENT STATUS" page — AquaPure / salt status-line processors.
//
// PROVENANCE OF THE LINE STRINGS
// ------------------------------
// No recorded capture exercises the AquaPure %, salt-PPM, or "Check AquaPure"
// lines of the Equipment Status page, so these page lines are reconstructed
// from the documented OneTouch 16-character screen format: the label is
// left-justified and the value right-justified within the 16-column row,
// producing MULTIPLE internal spaces.  That format is evidenced throughout the
// existing fixtures — e.g. "AquaPure      ON" (test_factories_jandy_auxillary),
// "Pool Heat    ENA", "Aux1         OFF" (unit_test_onetouchdevice).  The
// processors used to assume a SINGLE space between tokens, so an anchored
// regex_match never fired on a real right-justified line; that is the bug these
// tests lock down.  Strings here are 16 columns wide exactly, like the real
// screen rows.
//=============================================================================

BOOST_FIXTURE_TEST_SUITE(PageProcessor_EquipmentStatus_AquaPure_TestSuite, Test::OneTouchDevice)

namespace
{
	// Build a minimal 12-line "EQUIPMENT STATUS" page (line 0 is the title that
	// routes the page to PageProcessor_EquipmentStatus) carrying a single status
	// line of interest at row 2.  Unused rows are 16 blanks, exactly as the real
	// screen pads them.
	Test::OneTouchDevice::TestPage MakeEquipmentStatusPage(const std::string& status_line)
	{
		return Test::OneTouchDevice::TestPage
		{
			{ 0x0, "EQUIPMENT STATUS" },
			{ 0x1, "                " },
			{ 0x2, status_line        },
			{ 0x3, "                " },
			{ 0x4, "                " },
			{ 0x5, "                " },
			{ 0x6, "                " },
			{ 0x7, "                " },
			{ 0x8, "                " },
			{ 0x9, "                " },
			{ 0xA, "                " },
			{ 0xB, "                " }
		};
	}
}

// "AquaPure     50%" (right-justified) must set the AquaPure chlorinator's duty
// cycle to 50.  Regression: the single-space regex failed on the multi-space
// real format.
BOOST_AUTO_TEST_CASE(AquaPurePercentage_RightJustifiedLine_SetsDutyCycle)
{
	using namespace Kernel::AuxillaryTraitsTypes;

	LoadAndSignalTestPage(MakeEquipmentStatusPage("AquaPure     50%"));

	auto chlorinators = DataHub().Chlorinators();
	BOOST_REQUIRE_EQUAL(chlorinators.size(), 1u);

	auto duty_cycle = chlorinators.front()->AuxillaryTraits.TryGet(DutyCycleTrait{});
	BOOST_REQUIRE(duty_cycle.has_value());
	BOOST_CHECK_EQUAL(duty_cycle.value(), static_cast<uint8_t>(50));
}

// "AquaPure    100%" (the three-digit boundary value) must also be accepted.
BOOST_AUTO_TEST_CASE(AquaPurePercentage_RightJustifiedLine_HundredPercent)
{
	using namespace Kernel::AuxillaryTraitsTypes;

	LoadAndSignalTestPage(MakeEquipmentStatusPage("AquaPure    100%"));

	auto chlorinators = DataHub().Chlorinators();
	BOOST_REQUIRE_EQUAL(chlorinators.size(), 1u);

	auto duty_cycle = chlorinators.front()->AuxillaryTraits.TryGet(DutyCycleTrait{});
	BOOST_REQUIRE(duty_cycle.has_value());
	BOOST_CHECK_EQUAL(duty_cycle.value(), static_cast<uint8_t>(100));
}

// "Salt    3200 PPM" (right-justified) must publish the salt level to the
// DataHub.  Regression: the single-space regex failed on the multi-space real
// format.
BOOST_AUTO_TEST_CASE(SaltLevelPPM_RightJustifiedLine_SetsSaltLevel)
{
	LoadAndSignalTestPage(MakeEquipmentStatusPage("Salt    3200 PPM"));

	BOOST_CHECK_EQUAL(DataHub().SaltLevel().value(), 3200.0);
}

// "Check AquaPure" (left-justified, trailing pad) flags the chlorinator health
// as a general fault.  This line already matched (its regex has no embedded
// value token) — pin it so a future edit cannot silently break it.
BOOST_AUTO_TEST_CASE(CheckAquaPure_Line_SetsHealthGeneralFault)
{
	using namespace Kernel::AuxillaryTraitsTypes;

	LoadAndSignalTestPage(MakeEquipmentStatusPage("Check AquaPure  "));

	auto chlorinators = DataHub().Chlorinators();
	BOOST_REQUIRE_EQUAL(chlorinators.size(), 1u);

	auto health = chlorinators.front()->AuxillaryTraits.TryGet(ChlorinatorHealthTrait{});
	BOOST_REQUIRE(health.has_value());
	// ChlorinatorHealth has no ostream operator<<, so compare with == rather than
	// BOOST_CHECK_EQUAL (which would need to stream both operands on failure).
	BOOST_CHECK(health.value() == Kernel::ChlorinatorHealth::GeneralFault);
}

BOOST_AUTO_TEST_SUITE_END()
