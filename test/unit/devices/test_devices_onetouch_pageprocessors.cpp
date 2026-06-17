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

// =============================================================================
// OneTouch "Set AquaPure" page — configured Pool / Spa setpoint scrape.
//
// The page shows the CONFIGURED output setpoints (distinct from the instantaneous
// AQUARITE_Percent output): Pool % on line 3, Spa % on line 4 (verified vs
// onetouch_chlorinator.cap; see SETAQUAPURE_POOL_LINE / SETAQUAPURE_SPA_LINE).
// Reaching the page proves an SWG is installed, so the processor also creates the
// chlorinator auxillary if the AquaRite wire path has not already.
// =============================================================================

BOOST_FIXTURE_TEST_SUITE(PageProcessor_SetAquapure_TestSuite, Test::OneTouchDevice)

namespace
{
	// A 12-line "Set AQUAPURE" page (line 0 is the title that routes to
	// PageProcessor_SetAquapure) carrying the Pool % on line 3 and Spa % on line 4.
	Test::OneTouchDevice::TestPage MakeSetAquapurePage(const std::string& pool_line, const std::string& spa_line)
	{
		return Test::OneTouchDevice::TestPage
		{
			{ 0x0, "Set AQUAPURE    " },
			{ 0x1, "                " },
			{ 0x2, "                " },
			{ 0x3, pool_line          },
			{ 0x4, spa_line           },
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

// The configured Pool % (line 3) and Spa % (line 4) are scraped onto the chlorinator
// device, and the chlorinator auxillary is created from the scrape when none exists yet.
BOOST_AUTO_TEST_CASE(SetAquapure_ScrapesPoolAndSpaSetpoints)
{
	using namespace Kernel::AuxillaryTraitsTypes;

	LoadAndSignalTestPage(MakeSetAquapurePage("Set Pool to:  45%", "Set Spa to:   50%"));

	auto chlorinators = DataHub().Chlorinators();
	BOOST_REQUIRE_EQUAL(chlorinators.size(), 1u);

	auto& device = chlorinators.front();

	auto label = device->AuxillaryTraits.TryGet(LabelTrait{});
	BOOST_REQUIRE(label.has_value());
	BOOST_CHECK_EQUAL(label.value(), std::string{ "AquaPure" });

	auto pool = device->AuxillaryTraits.TryGet(ChlorinatorPoolSetpointTrait{});
	BOOST_REQUIRE(pool.has_value());
	BOOST_CHECK_EQUAL(pool.value(), static_cast<uint8_t>(45));

	auto spa = device->AuxillaryTraits.TryGet(ChlorinatorSpaSetpointTrait{});
	BOOST_REQUIRE(spa.has_value());
	BOOST_CHECK_EQUAL(spa.value(), static_cast<uint8_t>(50));
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// PageProcessor_System - home-page temperatures parsed by area, not line (OBS-04)
//
// The home page lists temperatures labelled by area ("Air"/"Pool"/"Spa"); the line
// they occupy is model-dependent. A dual-equipment panel lists both Pool Pump and Spa
// Pump, which pushes the air-temp line down to line 7 (vs line 6 on a shared-equipment
// panel). Previously air temp was read from a fixed line 6, so it came back blank on
// dual-equipment models -- it must be matched by area instead.
// =============================================================================

BOOST_FIXTURE_TEST_SUITE(PageProcessor_System_TestSuite, Test::OneTouchDevice)

BOOST_AUTO_TEST_CASE(SystemPage_DualEquipmentLayout_AirTempOnLine7)
{
	// RS-2/10 Dual home page: both pumps listed, so the air-temp line sits on line 7.
	const TestPage dual_home =
	{
		{ 0x0, "                " },
		{ 0x1, "                " },
		{ 0x2, "                " },
		{ 0x3, "Spa Mode     OFF" },
		{ 0x4, "                " },
		{ 0x5, "Pool Pump     ON" },
		{ 0x6, "Spa Pump     OFF" },
		{ 0x7, "    Air 72`F    " },
		{ 0x8, "                " },
		{ 0x9, "Equipment ON/OFF" },
		{ 0xA, "OneTouch  ON/OFF" },
		{ 0xB, "   Menu / Help  " }
	};

	LoadAndSignalTestPage(dual_home);

	auto air = DataHub().AirTemp();
	BOOST_REQUIRE(air.has_value());
	BOOST_CHECK_CLOSE(air.value().InFahrenheit().value(), 72.0, 1.0);
}

BOOST_AUTO_TEST_CASE(SystemPage_SharedEquipmentLayout_AirTempOnLine6)
{
	// Shared-equipment / single-body home page: air temp on line 6 (regression guard so
	// the by-area scan still handles the original layout).
	const TestPage shared_home =
	{
		{ 0x0, "                " },
		{ 0x1, "                " },
		{ 0x2, "                " },
		{ 0x3, "                " },
		{ 0x4, "                " },
		{ 0x5, "Filter Pump  OFF" },
		{ 0x6, "    Air 22`C    " },
		{ 0x7, "                " },
		{ 0x8, "                " },
		{ 0x9, "Equipment ON/OFF" },
		{ 0xA, "OneTouch  ON/OFF" },
		{ 0xB, "   Menu / Help  " }
	};

	LoadAndSignalTestPage(shared_home);

	auto air = DataHub().AirTemp();
	BOOST_REQUIRE(air.has_value());
	BOOST_CHECK_CLOSE(air.value().InCelsius().value(), 22.0, 1.0);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// PageProcessor_SetTemperature - heat setpoints (OBS-03)
//
// The Set Temperature page lists "Pool Heat" / "Spa Heat" setpoints. The labels are
// two words and a spa setpoint can be 100`F+ (three digits); previously the temperature
// converter rejected both, so the setpoints came back null on Combo models even though
// the page rendered them. Parse them by area.
// =============================================================================

BOOST_FIXTURE_TEST_SUITE(PageProcessor_SetTemperature_TestSuite, Test::OneTouchDevice)

BOOST_AUTO_TEST_CASE(SetTemperaturePage_DecodesHeatSetpoints_MultiWordLabelAndThreeDigit)
{
	const TestPage set_temp =
	{
		{ 0x0, "    Set Temp    " },
		{ 0x1, "                " },
		{ 0x2, "Pool Heat   80`F" },
		{ 0x3, "Spa Heat   102`F" },
		{ 0x4, "                " },
		{ 0x5, "Maintain     OFF" },
		{ 0x6, "Hours  12AM-12AM" },
		{ 0x7, "                " },
		{ 0x8, "Highlight an    " },
		{ 0x9, "item and press  " },
		{ 0xA, "Select          " },
		{ 0xB, "                " }
	};

	LoadAndSignalTestPage(set_temp);

	auto pool_sp = DataHub().PoolTempSetpoint();
	BOOST_REQUIRE(pool_sp.has_value());
	BOOST_CHECK_CLOSE(pool_sp.value().InFahrenheit().value(), 80.0, 1.0);

	auto spa_sp = DataHub().SpaTempSetpoint();
	BOOST_REQUIRE(spa_sp.has_value());
	BOOST_CHECK_CLOSE(spa_sp.value().InFahrenheit().value(), 102.0, 1.0);
}

BOOST_AUTO_TEST_SUITE_END()
