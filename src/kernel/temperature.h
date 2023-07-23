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
		using Celsius = boost::units::quantity<boost::units::absolute<boost::units::celsius::temperature>>;
		using Fahrenheit = boost::units::quantity<boost::units::absolute<boost::units::fahrenheit::temperature>>;
		using Kelvin = boost::units::quantity<boost::units::absolute<boost::units::si::temperature>>;

	public:
		Temperature(Celsius degrees_celsius);
		Temperature(Fahrenheit degrees_fahrenheit);

	public:
		Celsius InCelsius() const;
		Fahrenheit InFahrenheit() const;

	public:
		static Temperature ConvertToTemperatureInCelsius(double degrees_celsius);
		static Temperature ConvertToTemperatureInFahrenheit(double degrees_fahrenheit);

	private:
		Kelvin m_TempInKelvin;
	};

}
// namespace AqualinkAutomate::Kernel
