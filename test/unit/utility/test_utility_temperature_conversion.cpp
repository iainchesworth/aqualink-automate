#include <cstdint>

#include <boost/test/unit_test.hpp>

#include <nlohmann/json.hpp>

#include "kernel/temperature.h"
#include "utility/json_serialization_helpers.h"
#include "utility/temperature_conversion.h"

using namespace AqualinkAutomate;

BOOST_AUTO_TEST_SUITE(TestSuite_UtilityTemperatureConversion)

BOOST_AUTO_TEST_CASE(Test_CelsiusToFahrenheit_KnownPoints)
{
	BOOST_CHECK_CLOSE(32.0, Utility::CelsiusToFahrenheit(0.0), 1e-9);
	BOOST_CHECK_CLOSE(212.0, Utility::CelsiusToFahrenheit(100.0), 1e-9);
	BOOST_CHECK_CLOSE(-40.0, Utility::CelsiusToFahrenheit(-40.0), 1e-9);
	BOOST_CHECK_CLOSE(77.0, Utility::CelsiusToFahrenheit(25.0), 1e-9);
}

BOOST_AUTO_TEST_CASE(Test_CelsiusToWireSetpoint_FahrenheitUnits)
{
	// 25C -> 77F (exact); 28C -> 82.4F -> rounds to 82.
	BOOST_CHECK_EQUAL(static_cast<std::uint8_t>(77), Utility::CelsiusToWireSetpoint(25.0, Kernel::TemperatureUnits::Fahrenheit));
	BOOST_CHECK_EQUAL(static_cast<std::uint8_t>(82), Utility::CelsiusToWireSetpoint(28.0, Kernel::TemperatureUnits::Fahrenheit));
}

BOOST_AUTO_TEST_CASE(Test_CelsiusToWireSetpoint_CelsiusUnitsAreSentAsIs)
{
	BOOST_CHECK_EQUAL(static_cast<std::uint8_t>(25), Utility::CelsiusToWireSetpoint(25.0, Kernel::TemperatureUnits::Celsius));
	// Rounds to nearest whole degree.
	BOOST_CHECK_EQUAL(static_cast<std::uint8_t>(26), Utility::CelsiusToWireSetpoint(25.6, Kernel::TemperatureUnits::Celsius));
}

BOOST_AUTO_TEST_CASE(Test_CelsiusToWireSetpoint_ClampsBelowZero)
{
	// A strongly negative Celsius value in Celsius units would round to a negative
	// double; the helper must clamp to 0 BEFORE the uint8_t cast (no UB / wraparound).
	BOOST_CHECK_EQUAL(static_cast<std::uint8_t>(0), Utility::CelsiusToWireSetpoint(-100.0, Kernel::TemperatureUnits::Celsius));
	// In Fahrenheit units -40C is -40F which must also clamp to 0.
	BOOST_CHECK_EQUAL(static_cast<std::uint8_t>(0), Utility::CelsiusToWireSetpoint(-40.0, Kernel::TemperatureUnits::Fahrenheit));
}

BOOST_AUTO_TEST_CASE(Test_CelsiusToWireSetpoint_ClampsAbove255)
{
	// A huge Celsius value would exceed the uint8_t domain; the clamp keeps it at 255.
	BOOST_CHECK_EQUAL(static_cast<std::uint8_t>(255), Utility::CelsiusToWireSetpoint(1000.0, Kernel::TemperatureUnits::Celsius));
	BOOST_CHECK_EQUAL(static_cast<std::uint8_t>(255), Utility::CelsiusToWireSetpoint(1000.0, Kernel::TemperatureUnits::Fahrenheit));
}

//
// Direction 1: the controller reports CELSIUS and we expose Fahrenheit.
//
// The Jandy water sensors are whole-degree readings; verify both readouts of the stored
// Temperature land on the textbook C->F result regardless of the internal Kelvin storage.
//
BOOST_AUTO_TEST_CASE(Test_Temperature_ControllerReportsCelsius_ConvertsToFahrenheit)
{
	const auto t = Kernel::Temperature::ConvertToTemperatureInCelsius(38.0);
	BOOST_CHECK_CLOSE(38.0, t.InCelsius().value(), 1e-6);
	BOOST_CHECK_CLOSE(100.4, t.InFahrenheit().value(), 1e-6);

	// A few more textbook reference points (freezing, boiling, the -40 crossover, room temp).
	BOOST_CHECK_CLOSE(32.0, Kernel::Temperature::ConvertToTemperatureInCelsius(0.0).InFahrenheit().value(), 1e-6);
	BOOST_CHECK_CLOSE(212.0, Kernel::Temperature::ConvertToTemperatureInCelsius(100.0).InFahrenheit().value(), 1e-6);
	BOOST_CHECK_CLOSE(-40.0, Kernel::Temperature::ConvertToTemperatureInCelsius(-40.0).InFahrenheit().value(), 1e-6);
	BOOST_CHECK_CLOSE(77.0, Kernel::Temperature::ConvertToTemperatureInCelsius(25.0).InFahrenheit().value(), 1e-6);
}

//
// Direction 2: the controller reports FAHRENHEIT (the common Jandy default) and we expose Celsius.
//
BOOST_AUTO_TEST_CASE(Test_Temperature_ControllerReportsFahrenheit_ConvertsToCelsius)
{
	const auto t = Kernel::Temperature::ConvertToTemperatureInFahrenheit(100.0);
	BOOST_CHECK_CLOSE(100.0, t.InFahrenheit().value(), 1e-6);
	BOOST_CHECK_CLOSE(37.77777777777778, t.InCelsius().value(), 1e-6);

	BOOST_CHECK_CLOSE(0.0, Kernel::Temperature::ConvertToTemperatureInFahrenheit(32.0).InCelsius().value(), 1e-6);
	BOOST_CHECK_CLOSE(100.0, Kernel::Temperature::ConvertToTemperatureInFahrenheit(212.0).InCelsius().value(), 1e-6);
	BOOST_CHECK_CLOSE(-40.0, Kernel::Temperature::ConvertToTemperatureInFahrenheit(-40.0).InCelsius().value(), 1e-6);
	BOOST_CHECK_CLOSE(22.222222222222, Kernel::Temperature::ConvertToTemperatureInFahrenheit(72.0).InCelsius().value(), 1e-6);
}

//
// The internal Kelvin round-trip injects sub-ULP float noise (38.0C surfaces as
// 100.39999999999992F). RoundTemperatureForDisplay must collapse that to the nearest tenth.
//
BOOST_AUTO_TEST_CASE(Test_RoundTemperatureForDisplay_CollapsesFloatNoise)
{
	BOOST_CHECK_EQUAL(100.4, Utility::RoundTemperatureForDisplay(100.39999999999992));
	BOOST_CHECK_EQUAL(72.0, Utility::RoundTemperatureForDisplay(72.00000000000006));
	BOOST_CHECK_EQUAL(32.0, Utility::RoundTemperatureForDisplay(31.999999999999943));
	BOOST_CHECK_EQUAL(-0.4, Utility::RoundTemperatureForDisplay(-0.4000000000000341));
	BOOST_CHECK_EQUAL(22.2, Utility::RoundTemperatureForDisplay(22.222222222222285));
}

//
// End-to-end output precision: the serialized JSON the API/MQTT actually emit must read as the
// clean documented values (swagger spa_setpoint fahrenheit: 100.4), NOT the raw 100.39999999999992
// float. Asserting on the dumped string is what pins down "sensible precision".
//
BOOST_AUTO_TEST_CASE(Test_SerializeTemperature_OutputPrecisionIsSensible)
{
	// 38.0C -> 100.4F (the user's worked example).
	BOOST_CHECK_EQUAL(
		Utility::SerializeTemperature(Kernel::Temperature::ConvertToTemperatureInCelsius(38.0)).dump(),
		R"({"celsius":38.0,"fahrenheit":100.4})");

	// Half-degree Celsius setpoint round-trips cleanly too (swagger pool_setpoint example).
	BOOST_CHECK_EQUAL(
		Utility::SerializeTemperature(Kernel::Temperature::ConvertToTemperatureInCelsius(26.5)).dump(),
		R"({"celsius":26.5,"fahrenheit":79.7})");

	// Controller-reports-Fahrenheit path: the echoed Fahrenheit must not drift off its own value,
	// and the derived Celsius reads to one tenth.
	BOOST_CHECK_EQUAL(
		Utility::SerializeTemperature(Kernel::Temperature::ConvertToTemperatureInFahrenheit(72.0)).dump(),
		R"({"celsius":22.2,"fahrenheit":72.0})");

	// Whole-degree references at the extremes.
	BOOST_CHECK_EQUAL(
		Utility::SerializeTemperature(Kernel::Temperature::ConvertToTemperatureInCelsius(0.0)).dump(),
		R"({"celsius":0.0,"fahrenheit":32.0})");
}

//
// RoundToDecimalPlaces is the shared snap-to-resolution helper behind temperature, pH, and
// bandwidth-utilisation JSON output. Pin the two non-temperature bug classes it now guards.
//
BOOST_AUTO_TEST_CASE(Test_RoundToDecimalPlaces_pHFloat32Promotion)
{
	// A pH float32 of 7.1 promotes to the double 7.099999904632568; rounding to 1 dp must collapse
	// it back to the double that serializes as 7.1.
	const double promoted = static_cast<double>(7.1f);
	BOOST_CHECK(promoted != 7.1);                                  // the noise really is present
	BOOST_CHECK_EQUAL(7.1, Utility::RoundToDecimalPlaces(promoted, 1));
	BOOST_CHECK_EQUAL(7.4, Utility::RoundToDecimalPlaces(static_cast<double>(7.4f), 1));
	// An already-exact float32 (7.5) is unchanged.
	BOOST_CHECK_EQUAL(7.5, Utility::RoundToDecimalPlaces(static_cast<double>(7.5f), 1));
}

BOOST_AUTO_TEST_CASE(Test_RoundToDecimalPlaces_BandwidthPercentTwoPlaces)
{
	// Utilisation percentages are bytes/sec over capacity, so they carry long fractional tails.
	BOOST_CHECK_EQUAL(33.33, Utility::RoundToDecimalPlaces(100.0 / 3.0, 2));
	BOOST_CHECK_EQUAL(41.67, Utility::RoundToDecimalPlaces(100.0 * 5.0 / 12.0, 2));
	BOOST_CHECK_EQUAL(0.0, Utility::RoundToDecimalPlaces(0.0, 2));
	BOOST_CHECK_EQUAL(100.0, Utility::RoundToDecimalPlaces(100.0, 2));
}

BOOST_AUTO_TEST_SUITE_END()
