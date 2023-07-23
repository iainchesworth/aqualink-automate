#include <boost/units/base_units/temperature/conversions.hpp>

#include "kernel/temperature.h"

namespace AqualinkAutomate::Kernel
{

	Temperature::Temperature(Celsius degrees_celsius) :
		m_TempInKelvin(degrees_celsius)
	{
	}

	Temperature::Temperature(Fahrenheit degrees_fahrenheit) :
		m_TempInKelvin(degrees_fahrenheit)
	{
	}

	Temperature::Celsius Temperature::InCelsius() const
	{
		return Celsius(m_TempInKelvin);
	}

	Temperature::Fahrenheit Temperature::InFahrenheit() const
	{
		return Fahrenheit(m_TempInKelvin);
	}

	Temperature Temperature::ConvertToTemperatureInCelsius(double degrees_celsius)
	{
		return Temperature(degrees_celsius * Celsius::unit_type());
	}

	Temperature Temperature::ConvertToTemperatureInFahrenheit(double degrees_fahrenheit)
	{
		return Temperature(degrees_fahrenheit * Fahrenheit::unit_type());
	}

}
// namespace AqualinkAutomate::Kernel
