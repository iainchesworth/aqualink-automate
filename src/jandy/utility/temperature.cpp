#include <format>

#include <magic_enum.hpp>

#include "logging/logging.h"
#include "jandy/utility/temperature.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{

	Temperature::Temperature() noexcept :
		m_Temperature(0),
		m_TemperatureUnits(Units::Unknown),
		m_TemperatureArea()
	{
	}

	Temperature::Temperature(const std::string& temperature_string) noexcept :
		m_Temperature(0),
		m_TemperatureUnits(Units::Unknown),
		m_TemperatureArea()
	{
		ConvertStringToTemperature(temperature_string);
	}

	Temperature::Temperature(const Temperature& other) noexcept :
		m_Temperature(other.m_Temperature), 
		m_TemperatureUnits(other.m_TemperatureUnits), 
		m_TemperatureArea(other.m_TemperatureArea)
	{
	}

	Temperature::Temperature(Temperature&& other) noexcept :
		m_Temperature(std::move(other.m_Temperature)),
		m_TemperatureUnits(std::move(other.m_TemperatureUnits)),
		m_TemperatureArea(std::move(other.m_TemperatureArea))
	{
	}

	Temperature& Temperature::operator=(const Temperature& other) noexcept
	{
		if (this != &other) 
		{
			m_Temperature = other.m_Temperature;
			m_TemperatureUnits = other.m_TemperatureUnits;
			m_TemperatureArea = other.m_TemperatureArea;
		}

		return *this;
	}

	Temperature& Temperature::operator=(Temperature&& other) noexcept
	{
		if (this != &other) 
		{
			m_Temperature = std::move(other.m_Temperature);
			m_TemperatureUnits = std::move(other.m_TemperatureUnits);
			m_TemperatureArea = std::move(other.m_TemperatureArea);
		}

		return *this;
	}

	Temperature& Temperature::operator=(const std::string& temperature_string) noexcept
	{
		ConvertStringToTemperature(temperature_string);
		return *this;
	}

	uint8_t Temperature::operator()() const noexcept
	{
		return m_Temperature;
	}

	Temperature::Units Temperature::TemperatureUnits() const noexcept
	{
		return m_TemperatureUnits;
	}

	std::string Temperature::TemperatureArea() const noexcept
	{
		return m_TemperatureArea;
	}

	void Temperature::ConvertStringToTemperature(const std::string& temperature_string) noexcept
	{
		if (EXPECTED_STRING_LENGTH != temperature_string.length())
		{
			LogDebug(Channel::Devices, std::format("Failed to convert temperature; invalid string length: {}", temperature_string.length()));
		}
		else
		{
			const auto temperature_area = temperature_string.substr(TEMPERATURE_AREA_INDEX_START, TEMPERATURE_AREA_LENGTH);
			const auto temperature = temperature_string.substr(TEMPERATURE_INDEX_START, TEMPERATURE_LENGTH);
			const auto temperature_units = temperature_string[TEMPERATURE_UNITS_INDEX];

			uint32_t converted_temperature;

			auto [p, ec] = std::from_chars(temperature.data(), temperature.data() + temperature.size(), converted_temperature);
			if (std::errc() != ec)
			{
				LogDebug(Channel::Devices, std::format("Failed to convert temperature; could not convert to number: error -> {}", magic_enum::enum_name(ec)));
			}
			else if (converted_temperature > std::numeric_limits<uint8_t>::max())
			{
				LogDebug(Channel::Devices, std::format("Failed to convert temperature; value exceeded permitted range: value -> {}", converted_temperature));
			}
			else
			{
				m_Temperature = static_cast<uint8_t>(converted_temperature);
				m_TemperatureArea = temperature_area;

				switch (temperature_units)
				{
				case 'C':
					m_TemperatureUnits = Units::Celsius;
					break;
				case 'F':
					m_TemperatureUnits = Units::Farenheit;
					break;
				default:
					m_TemperatureUnits = Units::Unknown;
					break;
				}
			}
		}
	}

}
// namespace AqualinkAutomate::Utility
