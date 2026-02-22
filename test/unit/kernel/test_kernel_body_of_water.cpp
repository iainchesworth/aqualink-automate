#include <optional>
#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/units/systems/temperature/celsius.hpp>

#include "kernel/body_of_water.h"
#include "kernel/body_of_water_ids.h"
#include "kernel/temperature.h"

using namespace AqualinkAutomate::Kernel;

BOOST_AUTO_TEST_SUITE(BodyOfWater_TestSuite)

// =============================================================================
// Construction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_SetsIdAndLabel)
{
	BodyOfWater bow(BodyOfWaterIds::Pool, "Swimming Pool");
	BOOST_CHECK(bow.Id() == BodyOfWaterIds::Pool);
	BOOST_CHECK_EQUAL(bow.Label(), "Swimming Pool");
}

BOOST_AUTO_TEST_CASE(TestConstruction_DefaultsToInactive)
{
	BodyOfWater bow(BodyOfWaterIds::Spa, "Hot Tub");
	BOOST_CHECK(!bow.IsActive());
}

BOOST_AUTO_TEST_CASE(TestConstruction_NoTemperatureByDefault)
{
	BodyOfWater bow(BodyOfWaterIds::Pool, "Pool");
	BOOST_CHECK(!bow.CurrentTemp().has_value());
	BOOST_CHECK(!bow.TempSetpoint().has_value());
}

// =============================================================================
// IsActive
// =============================================================================

BOOST_AUTO_TEST_CASE(TestIsActive_SetTrue)
{
	BodyOfWater bow(BodyOfWaterIds::Pool, "Pool");
	bow.IsActive(true);
	BOOST_CHECK(bow.IsActive());
}

BOOST_AUTO_TEST_CASE(TestIsActive_SetFalse)
{
	BodyOfWater bow(BodyOfWaterIds::Pool, "Pool");
	bow.IsActive(true);
	bow.IsActive(false);
	BOOST_CHECK(!bow.IsActive());
}

// =============================================================================
// Temperatures
// =============================================================================

BOOST_AUTO_TEST_CASE(TestCurrentTemp_SetAndGet)
{
	BodyOfWater bow(BodyOfWaterIds::Pool, "Pool");
	auto temp = Temperature::ConvertToTemperatureInCelsius(28.5);
	bow.CurrentTemp(temp);

	BOOST_REQUIRE(bow.CurrentTemp().has_value());
	auto celsius = bow.CurrentTemp()->InCelsius();
	BOOST_CHECK_CLOSE(celsius.value(), 28.5, 0.01);
}

BOOST_AUTO_TEST_CASE(TestTempSetpoint_SetAndGet)
{
	BodyOfWater bow(BodyOfWaterIds::Spa, "Spa");
	auto setpoint = Temperature::ConvertToTemperatureInCelsius(38.0);
	bow.TempSetpoint(setpoint);

	BOOST_REQUIRE(bow.TempSetpoint().has_value());
	auto celsius = bow.TempSetpoint()->InCelsius();
	BOOST_CHECK_CLOSE(celsius.value(), 38.0, 0.01);
}

// =============================================================================
// Different IDs
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_SpaId)
{
	BodyOfWater bow(BodyOfWaterIds::Spa, "Spa");
	BOOST_CHECK(bow.Id() == BodyOfWaterIds::Spa);
}

BOOST_AUTO_TEST_CASE(TestConstruction_SharedId)
{
	BodyOfWater bow(BodyOfWaterIds::Shared, "Shared");
	BOOST_CHECK(bow.Id() == BodyOfWaterIds::Shared);
}

BOOST_AUTO_TEST_CASE(TestConstruction_UnknownId)
{
	BodyOfWater bow(BodyOfWaterIds::Unknown, "Unknown");
	BOOST_CHECK(bow.Id() == BodyOfWaterIds::Unknown);
}

BOOST_AUTO_TEST_SUITE_END()
