#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <ostream>
#include <span>
#include <string>
#include <vector>

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const std::array<uint8_t, 16>& obj);
	std::ostream& operator<<(std::ostream& os, const std::span<uint8_t>& obj);
	std::ostream& operator<<(std::ostream& os, const std::vector<uint8_t>& obj);

}
// namespace std

template<>
struct std::formatter<const std::array<uint8_t, 16>> : std::formatter<std::string>
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
struct std::formatter<std::array<uint8_t, 16>> : std::formatter<const std::array<uint8_t, 16>>
{
	template<typename Context>
	auto format(const std::array<uint8_t, 16>& arr, Context& ctx) const
	{
		return std::formatter<const std::array<uint8_t, 16>>::format(arr, ctx);
	}
};

template<>
struct std::formatter<const std::span<uint8_t>> : std::formatter<std::string>
{
	template<typename Context>
	auto format(const std::span<uint8_t>& span, Context& ctx) const
	{
		std::for_each_n(span.begin(), span.size(), [&ctx](auto& elem)
			{
				std::format_to(ctx.out(), "0x{:02x} ", elem);
			}
		);

		return ctx.out();
	}
};

template<>
struct std::formatter<std::span<uint8_t>> : std::formatter<const std::span<uint8_t>>
{
	template<typename Context>
	auto format(const std::span<uint8_t>& span, Context& ctx) const
	{
		return std::formatter<const std::span<uint8_t>>::format(span, ctx);
	}
};

template<>
struct std::formatter<const std::vector<uint8_t>> : std::formatter<std::string>
{
	template<typename Context>
	auto format(const std::vector<uint8_t>& vec, Context& ctx) const
	{
		std::for_each_n(vec.begin(), vec.size(), [&ctx](auto& elem)
			{
				std::format_to(ctx.out(), "0x{:02x} ", elem);
			}
		);

		return ctx.out();
	}
};

template<>
struct std::formatter<std::vector<uint8_t>> : std::formatter<const std::vector<uint8_t>>
{
	template<typename Context>
	auto format(const std::vector<uint8_t>& vec, Context& ctx) const
	{
		return std::formatter<const std::vector<uint8_t>>::format(vec, ctx);
	}
};
