#pragma once

#include <boost/units/quantity.hpp>
#include <boost/units/systems/si/temperature.hpp>
#include <boost/units/systems/temperature/celsius.hpp>
#include <boost/units/systems/temperature/fahrenheit.hpp>

namespace AqualinkAutomate::Kernel
{

	class Temperature
	{
	public:
		Temperature(boost::units::quantity<boost::units::celsius::temperature> degrees_celsius);
		Temperature(boost::units::quantity<boost::units::fahrenheit::temperature> degrees_fahrenheit);

	public:
		boost::units::quantity<boost::units::celsius::temperature> InCelsius() const;
		boost::units::quantity<boost::units::fahrenheit::temperature> InFahrenheit() const;

	public:
		static Temperature ConvertToTemperatureInCelsius(double degrees_celsius);
		static Temperature ConvertToTemperatureInFahrenheit(double degrees_fahrenheit);

	private:
		boost::units::quantity<boost::units::si::temperature> m_TempInKelvin;
	};

}
// namespace AqualinkAutomate::Kernel
