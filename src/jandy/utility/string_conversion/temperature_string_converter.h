#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <tuple>

#include <boost/regex.hpp>
#include <boost/system/error_code.hpp>
#include <tl/expected.hpp>

#include "jandy/errors/string_conversion_errors.h"
#include "kernel/temperature.h"

using namespace AqualinkAutomate::ErrorCodes;

namespace AqualinkAutomate::Utility
{

	class TemperatureStringConverter
	{
		static const std::string REGEX_PATTERN;
		static const boost::regex REGEX_PARSER;

		static const uint8_t MAXIMUM_STRING_LENGTH = 16;
		static const uint8_t MINIMUM_STRING_LENGTH = 7;

	public:
		TemperatureStringConverter() noexcept;
		TemperatureStringConverter(const std::string& temperature_string) noexcept;

	public:
		TemperatureStringConverter& operator=(const std::string& temperature_string) noexcept;

	public:
		tl::expected<Kernel::Temperature, boost::system::error_code> operator()() const noexcept;
		tl::expected<std::string, boost::system::error_code> TemperatureArea() const noexcept;

	private:
		void ConvertStringToTemperature(const std::string& temperature_string) noexcept;
		std::tuple<std::optional<std::string>, std::optional<std::string>, std::optional<std::string>> ValidateAndExtractData(const std::string& temperature_string) noexcept;

	private:
		Kernel::Temperature m_Temperature;
		std::string m_TemperatureArea;

	private:
		std::optional<ErrorCodes::StringConversion_ErrorCodes> m_ErrorOccurred;
	};

}
// namespace AqualinkAutomate::Utility
