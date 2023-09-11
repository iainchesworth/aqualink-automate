#include <algorithm>
#include <charconv>
#include <format>
#include <limits>

#include <magic_enum.hpp>

#include "jandy/utility/string_manipulation.h"
#include "jandy/utility/string_conversion/temperature.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{
	const std::string Temperature::REGEX_PATTERN{ R"(^([A-Za-z]{1,10})\s{1,10}(-?\d{1,2})`([CF])$)" };
	const boost::regex Temperature::REGEX_PARSER{ REGEX_PATTERN };

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
		const auto temperature_data = ValidateAndExtractData(temperature_string);

		const auto temperature_area = std::get<0>(temperature_data);
		const auto temperature = std::get<1>(temperature_data);
		const auto temperature_units = std::get<2>(temperature_data);

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
		boost::smatch match_results;

		if (MINIMUM_STRING_LENGTH > temperature_string.size() || MAXIMUM_STRING_LENGTH < temperature_string.size())
		{
			// Invalid string length...do nothing.
		}
		else if (!boost::regex_search(temperature_string, match_results, REGEX_PARSER))
		{
			// Invalid pattern match...do nothing.	
		}
		else if (3 > match_results.size())
		{
			// Insufficent resultset to pull groups from.
		}
		else 
		{
			return std::make_tuple<>(
				std::optional<std::string>(match_results[1]),
				std::optional<std::string>(match_results[2]),
				std::optional<std::string>(match_results[3])
			);
		}

		return std::make_tuple(std::nullopt, std::nullopt, std::nullopt);
	}

}
// namespace AqualinkAutomate::Utility
