#include <cstdint>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/jandy_device_types.h"
#include "jandy/formatters/jandy_device_formatters.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Devices;

//=============================================================================
// Device-id coverage for the RS-485 protocol gaps identified against the
// AqualinkD reference.  JandyDeviceType maps a raw destination byte to a
// DeviceClasses value; these tests pin the id->class mapping for the ranges
// the audit flagged so a future change cannot silently drop coverage.
//
//   * ePump extended ids   0xE0-0xE3 (Rev W panels)  -> Jandy_ePump_Ext
//   * Heater LX ids        0x38-0x3B                 -> LX_Heater
//   * Heater JXi ids       0x68-0x6B                 -> JXi_Heater
//   * Jandy light ids      0xF0-0xF4                 -> Jandy_Light
//   * Chemistry feeder ids 0x80-0x83                 -> Chemlink
//   * Chemistry analyzer   0x84-0x87                 -> Chem_Analyzer
//=============================================================================

BOOST_AUTO_TEST_SUITE(JandyDeviceTypeProtocolCoverageTestSuite)

// ---------------------------------------------------------------------------
// Task 2: ePump extended ids 0xE0-0xE3 (Rev W panels).
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(EPumpExtended_Ids_MapToEPumpExtClass)
{
	for (uint8_t id = 0xE0; id <= 0xE3; ++id)
	{
		JandyDeviceType device(id);
		BOOST_CHECK_EQUAL(device.Class(), DeviceClasses::Jandy_ePump_Ext);
		BOOST_CHECK_EQUAL(device.Id(), id);
	}
}

BOOST_AUTO_TEST_CASE(EPumpStandard_Ids_StillMapToEPumpClass)
{
	for (uint8_t id = 0x78; id <= 0x7B; ++id)
	{
		JandyDeviceType device(id);
		BOOST_CHECK_EQUAL(device.Class(), DeviceClasses::Jandy_ePump);
		BOOST_CHECK_EQUAL(device.Id(), id);
	}
}

// ---------------------------------------------------------------------------
// Task 3: heater id ranges LX (0x38-0x3B) and JXi (0x68-0x6B).
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(HeaterLX_Ids_MapToLXHeaterClass)
{
	for (uint8_t id = 0x38; id <= 0x3B; ++id)
	{
		JandyDeviceType device(id);
		BOOST_CHECK_EQUAL(device.Class(), DeviceClasses::LX_Heater);
		BOOST_CHECK_EQUAL(device.Id(), id);
	}
}

BOOST_AUTO_TEST_CASE(HeaterJXi_Ids_MapToJXiHeaterClass)
{
	for (uint8_t id = 0x68; id <= 0x6B; ++id)
	{
		JandyDeviceType device(id);
		BOOST_CHECK_EQUAL(device.Class(), DeviceClasses::JXi_Heater);
		BOOST_CHECK_EQUAL(device.Id(), id);
	}
}

// ---------------------------------------------------------------------------
// Task 1: Jandy light ids 0xF0-0xF4.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(JandyLight_Ids_MapToLightClass)
{
	for (uint8_t id = 0xF0; id <= 0xF4; ++id)
	{
		JandyDeviceType device(id);
		BOOST_CHECK_EQUAL(device.Class(), DeviceClasses::Jandy_Light);
		BOOST_CHECK_EQUAL(device.Id(), id);
	}
}

// ---------------------------------------------------------------------------
// Task 5: chemistry feeder (Chemlink) ids 0x80-0x83 and analyzer 0x84-0x87.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(ChemistryFeeder_Ids_MapToChemlinkClass)
{
	for (uint8_t id = 0x80; id <= 0x83; ++id)
	{
		JandyDeviceType device(id);
		BOOST_CHECK_EQUAL(device.Class(), DeviceClasses::Chemlink);
		BOOST_CHECK_EQUAL(device.Id(), id);
	}
}

BOOST_AUTO_TEST_CASE(ChemistryAnalyzer_Ids_MapToChemAnalyzerClass)
{
	for (uint8_t id = 0x84; id <= 0x87; ++id)
	{
		JandyDeviceType device(id);
		BOOST_CHECK_EQUAL(device.Class(), DeviceClasses::Chem_Analyzer);
		BOOST_CHECK_EQUAL(device.Id(), id);
	}
}

BOOST_AUTO_TEST_SUITE_END()
