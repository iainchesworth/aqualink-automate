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
		std::string Localised(const T& object)
		{
			// Default implementation for generic types.
			return std::string{};
		}

	public:
		std::string Localised(const Kernel::Temperature& object)
		{
			const std::string_view TEMPERATURE_DISPLAY_FORMAT{(Kernel::TemperatureUnits::Celsius == Kernel::TemperatureUnits::Celsius) ? "{:C}" : "{:F}" };
			return std::vformat(TEMPERATURE_DISPLAY_FORMAT, std::make_format_args(object));
		}
	};

}
// namespace AqualinkAutomate::Localisation
