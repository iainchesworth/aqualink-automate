#include <string>

#include <boost/test/unit_test.hpp>

#include "kernel/equipment_versions.h"

using namespace AqualinkAutomate::Kernel;

BOOST_AUTO_TEST_SUITE(EquipmentVersions_TestSuite)

// =============================================================================
// Default state
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDefault_FieldsEmpty)
{
	EquipmentVersions versions;
	BOOST_CHECK(versions.Fields.empty());
}

BOOST_AUTO_TEST_CASE(TestDefault_GetReturnsEmptyString)
{
	EquipmentVersions versions;
	BOOST_CHECK_EQUAL(versions.Get("Model"), "");
}

BOOST_AUTO_TEST_CASE(TestDefault_ModelNumberReturnsEmpty)
{
	EquipmentVersions versions;
	BOOST_CHECK_EQUAL(versions.ModelNumber(), "");
}

BOOST_AUTO_TEST_CASE(TestDefault_FirmwareRevisionReturnsEmpty)
{
	EquipmentVersions versions;
	BOOST_CHECK_EQUAL(versions.FirmwareRevision(), "");
}

// =============================================================================
// Set and Get
// =============================================================================

BOOST_AUTO_TEST_CASE(TestSet_ThenGet_ReturnsValue)
{
	EquipmentVersions versions;
	versions.Set("Model", "RS-8 Combo");
	BOOST_CHECK_EQUAL(versions.Get("Model"), "RS-8 Combo");
}

BOOST_AUTO_TEST_CASE(TestSet_MultipleFields)
{
	EquipmentVersions versions;
	versions.Set("Model", "RS-8 Combo");
	versions.Set("Type", "Rev AA");
	versions.Set("Revision", "5.2");

	BOOST_CHECK_EQUAL(versions.Get("Model"), "RS-8 Combo");
	BOOST_CHECK_EQUAL(versions.Get("Type"), "Rev AA");
	BOOST_CHECK_EQUAL(versions.Get("Revision"), "5.2");
	BOOST_CHECK_EQUAL(versions.Fields.size(), 3);
}

BOOST_AUTO_TEST_CASE(TestSet_OverwritesExistingValue)
{
	EquipmentVersions versions;
	versions.Set("Model", "RS-4 Only");
	versions.Set("Model", "RS-8 Combo");

	BOOST_CHECK_EQUAL(versions.Get("Model"), "RS-8 Combo");
	BOOST_CHECK_EQUAL(versions.Fields.size(), 1);
}

BOOST_AUTO_TEST_CASE(TestGet_NonexistentLabel_ReturnsEmpty)
{
	EquipmentVersions versions;
	versions.Set("Model", "RS-8 Combo");
	BOOST_CHECK_EQUAL(versions.Get("Nonexistent"), "");
}

// =============================================================================
// Convenience accessors
// =============================================================================

BOOST_AUTO_TEST_CASE(TestModelNumber_ReturnsModelField)
{
	EquipmentVersions versions;
	versions.Set("Model", "PD-8 Combo");
	BOOST_CHECK_EQUAL(versions.ModelNumber(), "PD-8 Combo");
}

BOOST_AUTO_TEST_CASE(TestFirmwareRevision_ReturnsRevisionField)
{
	EquipmentVersions versions;
	versions.Set("Revision", "6.0a");
	BOOST_CHECK_EQUAL(versions.FirmwareRevision(), "6.0a");
}

BOOST_AUTO_TEST_SUITE_END()
