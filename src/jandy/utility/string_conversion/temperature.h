#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <tuple>

#include <boost/system/error_code.hpp>

#include "jandy/errors/string_conversion_errors.h"

using namespace AqualinkAutomate::ErrorCodes;

namespace AqualinkAutomate::Utility
{

	class Temperature
	{
		static const uint8_t MAXIMUM_STRING_LENGTH = 16;
		static const uint8_t MINIMUM_STRING_LENGTH = 7;

	public:
		enum class Units 
		{
			Celsius,
			Farenheit,
			Unknown
		};

	public:
		Temperature() noexcept;
		Temperature(const std::string& temperature_string) noexcept;
		Temperature(const Temperature& other) noexcept;
		Temperature(Temperature&& other) noexcept;

	public:
		Temperature& operator=(const Temperature& other) noexcept;
		Temperature& operator=(Temperature&& other) noexcept;
		Temperature& operator=(const std::string& temperature_string) noexcept;

	public:
		std::expected<int8_t, boost::system::error_code> operator()() const noexcept;
		std::expected<Units, boost::system::error_code> TemperatureUnits() const noexcept;
		std::expected<std::string, boost::system::error_code> TemperatureArea() const noexcept;

	private:
		void ConvertStringToTemperature(const std::string& temperature_string) noexcept;
		std::tuple<std::optional<std::string>, std::optional<std::string>, std::optional<std::string>> ValidateAndExtractData(const std::string& temperature_string) noexcept;

	private:
		int8_t m_Temperature;
		Units m_TemperatureUnits;
		std::string m_TemperatureArea;

	private:
		std::optional<ErrorCodes::StringConversion_ErrorCodes> m_ErrorOccurred;
	};

}
// namespace AqualinkAutomate::Utility
