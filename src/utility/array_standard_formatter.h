#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <vector>

#include <boost/asio/buffer.hpp>

template<>
struct std::formatter<std::array<uint8_t, 16>> : std::formatter<std::string>
{
	template<typename Context>
	auto format(const std::array<uint8_t, 16>& arr, Context& context) const
	{
		std::for_each_n(arr.begin(), 16, [&context](auto& elem)
			{
				std::format_to(context.out(), "0x{:02x} ", elem);
			}
		);

		return context.out();
	}
};

template<>
struct std::formatter<std::span<uint8_t>> : std::formatter<std::string>
{
	template<typename Context>
	auto format(const std::span<uint8_t>& arr, Context& context) const
	{
		std::for_each_n(arr.begin(), arr.size(), [&context](auto& elem)
			{
				std::format_to(context.out(), "0x{:02x} ", elem);
			}
		);

		return context.out();
	}
};

template<>
struct std::formatter<std::vector<uint8_t>> : std::formatter<std::string>
{
	template<typename Context>
	auto format(const std::vector<uint8_t>& arr, Context& context) const
	{
		std::for_each_n(arr.begin(), arr.size(), [&context](auto& elem)
			{
				std::format_to(context.out(), "0x{:02x} ", elem);
			}
		);

		return context.out();
	}
};

namespace AqualinkAutomate::Utility
{

}
// using namespace AqualinkAutomate::Utility
