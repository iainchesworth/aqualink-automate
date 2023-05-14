#pragma once

#include <charconv>
#include <cstdint>
#include <limits>
#include <string>

namespace AqualinkAutomate::Utility
{

	class Temperature
	{
		static const uint8_t EXPECTED_STRING_LENGTH = 16;
		static const uint8_t TEMPERATURE_AREA_INDEX_START = 0;
		static const uint8_t TEMPERATURE_AREA_LENGTH = 9;
		static const uint8_t TEMPERATURE_INDEX_START = 9;
		static const uint8_t TEMPERATURE_LENGTH = 5;
		static const uint8_t TEMPERATURE_UNITS_INDEX = 15;

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
		uint8_t operator()() const noexcept;
		Units TemperatureUnits() const noexcept;
		std::string TemperatureArea() const noexcept;

	private:
		void ConvertStringToTemperature(const std::string& temperature_string) noexcept;

	private:
		uint8_t m_Temperature;
		Units m_TemperatureUnits;
		std::string m_TemperatureArea;
	};

}
// namespace AqualinkAutomate::Utility
