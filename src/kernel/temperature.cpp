#include <boost/units/base_units/temperature/conversions.hpp>

#include "kernel/temperature.h"

namespace AqualinkAutomate::Kernel
{

	Temperature::Temperature(boost::units::quantity<boost::units::celsius::temperature> degrees_celsius) :
		m_TempInKelvin(degrees_celsius)
	{
	}

	Temperature::Temperature(boost::units::quantity<boost::units::fahrenheit::temperature> degrees_fahrenheit) :
		m_TempInKelvin(degrees_fahrenheit)
	{
	}

	boost::units::quantity<boost::units::celsius::temperature> Temperature::InCelsius() const
	{
		return boost::units::quantity<boost::units::celsius::temperature>(m_TempInKelvin);
	}

	boost::units::quantity<boost::units::fahrenheit::temperature> Temperature::InFahrenheit() const
	{
		return boost::units::quantity<boost::units::fahrenheit::temperature>(m_TempInKelvin);
	}

	Temperature Temperature::ConvertToTemperatureInCelsius(double degrees_celsius)
	{
		return Temperature(degrees_celsius * boost::units::celsius::degrees);
	}

	Temperature Temperature::ConvertToTemperatureInFahrenheit(double degrees_fahrenheit)
	{
		return Temperature(degrees_fahrenheit * boost::units::fahrenheit::degrees);
	}

}
// namespace AqualinkAutomate::Kernel
