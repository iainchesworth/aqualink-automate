#include <charconv>
#include <format>
#include <limits>

#include <boost/regex.hpp>
#include <magic_enum.hpp>

#include "jandy/utility/string_manipulation.h"
#include "jandy/utility/string_conversion/temperature.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{

	Temperature::Temperature() noexcept :
		m_Temperature(0),
		m_TemperatureUnits(Units::Unknown),
		m_TemperatureArea(),
		m_ErrorOccurred(std::nullopt)
	{
	}

	Temperature::Temperature(const std::string& temperature_string) noexcept :
		m_Temperature(0),
		m_TemperatureUnits(Units::Unknown),
		m_TemperatureArea(),
		m_ErrorOccurred(std::nullopt)
	{
		ConvertStringToTemperature(TrimWhitespace(temperature_string));
	}

	Temperature::Temperature(const Temperature& other) noexcept :
		m_Temperature(other.m_Temperature), 
		m_TemperatureUnits(other.m_TemperatureUnits), 
		m_TemperatureArea(other.m_TemperatureArea),
		m_ErrorOccurred(other.m_ErrorOccurred)
	{
	}

	Temperature::Temperature(Temperature&& other) noexcept :
		m_Temperature(std::move(other.m_Temperature)),
		m_TemperatureUnits(std::move(other.m_TemperatureUnits)),
		m_TemperatureArea(std::move(other.m_TemperatureArea)),
		m_ErrorOccurred(std::move(other.m_ErrorOccurred))
	{
	}

	Temperature& Temperature::operator=(const Temperature& other) noexcept
	{
		if (this != &other) 
		{
			m_Temperature = other.m_Temperature;
			m_TemperatureUnits = other.m_TemperatureUnits;
			m_TemperatureArea = other.m_TemperatureArea;
			m_ErrorOccurred = other.m_ErrorOccurred;
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
			m_ErrorOccurred = std::move(other.m_ErrorOccurred);
		}

		return *this;
	}

	Temperature& Temperature::operator=(const std::string& temperature_string) noexcept
	{
		ConvertStringToTemperature(TrimWhitespace(temperature_string));
		return *this;
	}

	std::expected<int8_t, boost::system::error_code> Temperature::operator()() const noexcept
	{
		if (m_ErrorOccurred.has_value())
		{
			return std::unexpected<boost::system::error_code>(make_error_code(m_ErrorOccurred.value()));
		}

		return m_Temperature;
	}

	std::expected<Temperature::Units, boost::system::error_code> Temperature::TemperatureUnits() const noexcept
	{
		if (m_ErrorOccurred.has_value())
		{
			return std::unexpected<boost::system::error_code>(make_error_code(m_ErrorOccurred.value()));
		}

		return m_TemperatureUnits;
	}

	std::expected<std::string, boost::system::error_code> Temperature::TemperatureArea() const noexcept
	{
		if (m_ErrorOccurred.has_value())
		{
			return std::unexpected<boost::system::error_code>(make_error_code(m_ErrorOccurred.value()));
		}

		return m_TemperatureArea;
	}

	void Temperature::ConvertStringToTemperature(const std::string& temperature_string) noexcept
	{
		const auto [temperature_area, temperature, temperature_units] = ValidateAndExtractData(temperature_string);
		if (temperature_area && temperature && temperature_units)
		{
			int32_t converted_temperature;

			auto [p, ec] = std::from_chars((*temperature).data(), (*temperature).data() + (*temperature).size(), converted_temperature);
			if (std::errc() != ec)
			{
				LogDebug(Channel::Devices, std::format("Failed to convert temperature; could not convert to number: error -> {}", magic_enum::enum_name(ec)));
				m_ErrorOccurred = ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
			}
			else if (converted_temperature > std::numeric_limits<decltype(m_Temperature)>::max())
			{
				LogDebug(Channel::Devices, std::format("Failed to convert temperature; value exceeded maximum permitted: value -> {}", converted_temperature));
				m_ErrorOccurred = ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
			}
			else if (converted_temperature < std::numeric_limits<decltype(m_Temperature)>::min())
			{
				LogDebug(Channel::Devices, std::format("Failed to convert temperature; value exceeded minimum permitted: value -> {}", converted_temperature));
				m_ErrorOccurred = ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
			}
			else
			{
				m_Temperature = static_cast<decltype(m_Temperature)>(converted_temperature);
				m_TemperatureArea = TrimWhitespace(*temperature_area);

				switch ((*temperature_units)[0])
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
		else 
		{
			LogDebug(Channel::Devices, std::format("Failed to convert temperature; malformed input -> {}", temperature_string));
			m_ErrorOccurred = ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
		}
	}

	std::tuple<std::optional<std::string>, std::optional<std::string>, std::optional<std::string>> Temperature::ValidateAndExtractData(const std::string& temperature_string)  noexcept
	{
		if (MINIMUM_STRING_LENGTH > temperature_string.size() || MAXIMUM_STRING_LENGTH < temperature_string.size())
		{
			return { std::nullopt, std::nullopt, std::nullopt };
		}

		boost::regex re("^([A-Za-z]{1,10})\\s{1,10}(-?\\d{1,2})`([CF])$");
		boost::smatch match;

		if (boost::regex_match(temperature_string, match, re))
		{
			return { match[1].str(), match[2].str(), match[3].str() };
		}
		else 
		{
			return { std::nullopt, std::nullopt, std::nullopt };
		}
	}

}
// namespace AqualinkAutomate::Utility
