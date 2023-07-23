#include <algorithm>
#include <charconv>
#include <format>
#include <limits>

#include <magic_enum.hpp>
#include <re2/re2.h>

#include "jandy/utility/string_manipulation.h"
#include "jandy/utility/string_conversion/temperature.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{

	Temperature::Temperature() noexcept :
		m_Temperature(Kernel::Temperature::ConvertToTemperatureInCelsius(0)),
		m_TemperatureArea(),
		m_ErrorOccurred(std::nullopt)
	{
	}

	Temperature::Temperature(const std::string& temperature_string) noexcept :
		m_Temperature(Kernel::Temperature::ConvertToTemperatureInCelsius(0)),
		m_TemperatureArea(),
		m_ErrorOccurred(std::nullopt)
	{
		ConvertStringToTemperature(TrimWhitespace(temperature_string));
	}

	Temperature& Temperature::operator=(const std::string& temperature_string) noexcept
	{
		ConvertStringToTemperature(TrimWhitespace(temperature_string));
		return *this;
	}

	tl::expected<Kernel::Temperature, boost::system::error_code> Temperature::operator()() const noexcept
	{
		if (m_ErrorOccurred.has_value())
		{
			return tl::unexpected<boost::system::error_code>(make_error_code(m_ErrorOccurred.value()));
		}

		return m_Temperature;
	}

	tl::expected<std::string, boost::system::error_code> Temperature::TemperatureArea() const noexcept
	{
		if (m_ErrorOccurred.has_value())
		{
			return tl::unexpected<boost::system::error_code>(make_error_code(m_ErrorOccurred.value()));
		}

		return m_TemperatureArea;
	}

	void Temperature::ConvertStringToTemperature(const std::string& temperature_string) noexcept
	{
		const auto [temperature_area, temperature, temperature_units] = ValidateAndExtractData(temperature_string);
		if (temperature_area && temperature && temperature_units)
		{
			int32_t converted_temperature;

			auto [_, ec] = std::from_chars((*temperature).data(), (*temperature).data() + (*temperature).size(), converted_temperature);
			if (std::errc() != ec)
			{
				LogDebug(Channel::Devices, std::format("Failed to convert temperature; could not convert to number: error -> {}", magic_enum::enum_name(ec)));
				m_ErrorOccurred = ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
			}
			else
			{
				switch ((*temperature_units)[0])
				{
				case 'C':
					m_Temperature = Kernel::Temperature::ConvertToTemperatureInCelsius(static_cast<double>(converted_temperature));
					break;
				case 'F':
					m_Temperature = Kernel::Temperature::ConvertToTemperatureInFahrenheit(static_cast<double>(converted_temperature));
					break;
				default:
					m_ErrorOccurred = ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
					break;
				}

				m_TemperatureArea = TrimWhitespace(*temperature_area);
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

		re2::RE2 re("^([A-Za-z]{1,10})\\s{1,10}(-?\\d{1,2})`([CF])$");
		std::string match1, match2, match3;

		if (re2::RE2::FullMatch(temperature_string, re, &match1, &match2, &match3))
		{
			return { match1, match2, match3 };
		}
		else 
		{
			return { std::nullopt, std::nullopt, std::nullopt };
		}
	}

}
// namespace AqualinkAutomate::Utility
