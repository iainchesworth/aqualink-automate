#pragma once

#include <format>
#include <string>
#include <string_view>

#include "formatters/temperature_formatter.h"
#include "kernel/temperature.h"

namespace AqualinkAutomate::Localisation
{

	class TranslationsAndUnitsFormatter
	{

	private:
		TranslationsAndUnitsFormatter();
		~TranslationsAndUnitsFormatter() = default;

	public:
		TranslationsAndUnitsFormatter(const TranslationsAndUnitsFormatter&) = delete;
		TranslationsAndUnitsFormatter& operator=(const TranslationsAndUnitsFormatter&) = delete;
		TranslationsAndUnitsFormatter(TranslationsAndUnitsFormatter&&) = delete;
		TranslationsAndUnitsFormatter& operator=(TranslationsAndUnitsFormatter&&) = delete;

	public:
		static TranslationsAndUnitsFormatter& Instance();

	public:
		template<typename T>
		constexpr std::string Localised(const T& object) const
		{
			// Default implementation for generic types.
			return std::string{};
		}

	public:
		constexpr std::string Localised(const Kernel::Temperature& object) const
		{
			const Kernel::TemperatureUnits temp_format{ Kernel::TemperatureUnits::Celsius };
			
			switch (temp_format)
			{
			case Kernel::TemperatureUnits::Celsius:
				return std::format("{:C}", object);

			case Kernel::TemperatureUnits::Fahrenheit:
				return std::format("{:F}", object);

			default:
				///FIXME - add a specific-to-localisation exception here.
				throw;
			}
		}
	};

}
// namespace AqualinkAutomate::Localisation
