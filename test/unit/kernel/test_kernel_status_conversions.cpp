#include <boost/test/unit_test.hpp>

#include "kernel/auxillary_devices/auxillary_status.h"
#include "kernel/auxillary_devices/chlorinator_status.h"
#include "kernel/auxillary_devices/heater_status.h"
#include "kernel/auxillary_devices/pump_status.h"

using namespace AqualinkAutomate::Kernel;

BOOST_AUTO_TEST_SUITE(KernelStatusConversions_TestSuite)

// =============================================================================
// ConvertToChlorinatorStatus
// =============================================================================

BOOST_AUTO_TEST_CASE(TestChlorinatorStatus_On)
{
	auto result = ConvertToChlorinatorStatus(AuxillaryStatuses::On);
	BOOST_CHECK(result == ChlorinatorStatuses::On);
}

BOOST_AUTO_TEST_CASE(TestChlorinatorStatus_Off)
{
	auto result = ConvertToChlorinatorStatus(AuxillaryStatuses::Off);
	BOOST_CHECK(result == ChlorinatorStatuses::Off);
}

BOOST_AUTO_TEST_CASE(TestChlorinatorStatus_Enabled_MapsToUnknown)
{
	auto result = ConvertToChlorinatorStatus(AuxillaryStatuses::Enabled);
	BOOST_CHECK(result == ChlorinatorStatuses::Unknown);
}

BOOST_AUTO_TEST_CASE(TestChlorinatorStatus_Pending_MapsToUnknown)
{
	auto result = ConvertToChlorinatorStatus(AuxillaryStatuses::Pending);
	BOOST_CHECK(result == ChlorinatorStatuses::Unknown);
}

BOOST_AUTO_TEST_CASE(TestChlorinatorStatus_Unknown_MapsToUnknown)
{
	auto result = ConvertToChlorinatorStatus(AuxillaryStatuses::Unknown);
	BOOST_CHECK(result == ChlorinatorStatuses::Unknown);
}

// =============================================================================
// ConvertToHeaterStatus
// =============================================================================

BOOST_AUTO_TEST_CASE(TestHeaterStatus_On_MapsToHeating)
{
	auto result = ConvertToHeaterStatus(AuxillaryStatuses::On);
	BOOST_CHECK(result == HeaterStatuses::Heating);
}

BOOST_AUTO_TEST_CASE(TestHeaterStatus_Off)
{
	auto result = ConvertToHeaterStatus(AuxillaryStatuses::Off);
	BOOST_CHECK(result == HeaterStatuses::Off);
}

BOOST_AUTO_TEST_CASE(TestHeaterStatus_Enabled)
{
	auto result = ConvertToHeaterStatus(AuxillaryStatuses::Enabled);
	BOOST_CHECK(result == HeaterStatuses::Enabled);
}

BOOST_AUTO_TEST_CASE(TestHeaterStatus_Pending_MapsToUnknown)
{
	auto result = ConvertToHeaterStatus(AuxillaryStatuses::Pending);
	BOOST_CHECK(result == HeaterStatuses::Unknown);
}

BOOST_AUTO_TEST_CASE(TestHeaterStatus_Unknown_MapsToUnknown)
{
	auto result = ConvertToHeaterStatus(AuxillaryStatuses::Unknown);
	BOOST_CHECK(result == HeaterStatuses::Unknown);
}

// =============================================================================
// ConvertToPumpStatus
// =============================================================================

BOOST_AUTO_TEST_CASE(TestPumpStatus_On_MapsToRunning)
{
	auto result = ConvertToPumpStatus(AuxillaryStatuses::On);
	BOOST_CHECK(result == PumpStatuses::Running);
}

BOOST_AUTO_TEST_CASE(TestPumpStatus_Off)
{
	auto result = ConvertToPumpStatus(AuxillaryStatuses::Off);
	BOOST_CHECK(result == PumpStatuses::Off);
}

BOOST_AUTO_TEST_CASE(TestPumpStatus_Enabled_MapsToUnknown)
{
	auto result = ConvertToPumpStatus(AuxillaryStatuses::Enabled);
	BOOST_CHECK(result == PumpStatuses::Unknown);
}

BOOST_AUTO_TEST_CASE(TestPumpStatus_Pending_MapsToUnknown)
{
	auto result = ConvertToPumpStatus(AuxillaryStatuses::Pending);
	BOOST_CHECK(result == PumpStatuses::Unknown);
}

BOOST_AUTO_TEST_CASE(TestPumpStatus_Unknown_MapsToUnknown)
{
	auto result = ConvertToPumpStatus(AuxillaryStatuses::Unknown);
	BOOST_CHECK(result == PumpStatuses::Unknown);
}

BOOST_AUTO_TEST_SUITE_END()
