#pragma once

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

template<>
struct std::formatter<AqualinkAutomate::Devices::JandyDeviceId> : std::formatter<std::string>
{
	template<typename FormatContext>
	auto format(const AqualinkAutomate::Devices::JandyDeviceId& device_id, FormatContext& ctx) const
	{
		return std::vformat_to(ctx.out(), "0x{:02x}", std::make_format_args(static_cast<uint8_t>(device_id())));
	}
};

template<>
struct std::formatter<AqualinkAutomate::Devices::JandyDeviceType> : std::formatter<std::string>
{
	template<typename FormatContext>
	auto format(const AqualinkAutomate::Devices::JandyDeviceType& device_type, FormatContext& ctx) const
	{
		return std::vformat_to(ctx.out(), "{}", std::make_format_args(device_type.Id()));
	}
};
