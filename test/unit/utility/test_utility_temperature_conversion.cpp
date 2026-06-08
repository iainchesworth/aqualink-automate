#include <cstdint>

#include <boost/test/unit_test.hpp>

#include "kernel/temperature.h"
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

BOOST_AUTO_TEST_SUITE_END()
