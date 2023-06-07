#pragma once

#include <cstdint>
#include <format>
#include <iostream>
#include <string>

#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Devices::JandyDeviceId& obj);
	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Devices::JandyDeviceType& obj);

}
// namespace std

template<std::derived_from<AqualinkAutomate::Devices::JandyDeviceId> Derived, typename CharT>
struct std::formatter<Derived, CharT> : std::formatter<std::string>
{
	template<typename Context>
	auto format(Derived& device_id, Context& context) const
	{
		static constexpr uint8_t MINIMIM_CONVERTABLE_ID = 0;
		static constexpr uint8_t MAXIMUM_CONVERTABLE_ID = 255;
		static constexpr uint8_t CONVERTED_STRING_LENGTH = 5;  // 4 characters plus a NUL i.e. device id is in range 0x00 .. 0xFF
		
		char device_id_string[CONVERTED_STRING_LENGTH];

		if ((MINIMIM_CONVERTABLE_ID > device_id()) || (MAXIMUM_CONVERTABLE_ID < device_id()))
		{
			device_id_string[0] = '\0';
		}
		else
		{
			uint8_t val = static_cast<uint8_t>(device_id());

			device_id_string[0] = '0';
			device_id_string[1] = 'x';
			device_id_string[2] = digits[val >> 4];  // First hexadecimal digit
			device_id_string[3] = digits[val & 0xF]; // Second hexadecimal digit
			device_id_string[4] = '\0';				 // NULL terminator
		}

		return std::formatter<std::string>::format(device_id_string, context);
	}

private:
	constexpr static char digits[16] = 
	{
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
	};
};

template<std::derived_from<AqualinkAutomate::Devices::JandyDeviceType> Derived, typename CharT>
struct std::formatter<Derived, CharT> : std::formatter<std::string>
{
	template<typename Context>
	auto format(Derived& device_type, Context& context) const
	{
		return std::formatter<std::string>::format(device_type.Id(), context);
	}
};
