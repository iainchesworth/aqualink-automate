#pragma once

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
struct std::formatter<const std::array<uint8_t, 16>>
{
	constexpr auto parse(std::format_parse_context& ctx)
	{
		return ctx.begin();
	}

	template<typename Context>
	auto format(const std::array<uint8_t, 16>& arr, Context& ctx) const
	{
		auto out = ctx.out();
		bool first = true;

		for (const auto& elem : arr)
		{
			if (!first)
			{
				*out++ = ' ';
			}
			first = false;
			out = std::format_to(out, "0x{:02x}", elem);
		}

		return out;
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
struct std::formatter<const std::span<uint8_t>>
{
	constexpr auto parse(std::format_parse_context& ctx)
	{
		return ctx.begin();
	}

	template<typename Context>
	auto format(const std::span<uint8_t>& span, Context& ctx) const
	{
		auto out = ctx.out();
		bool first = true;

		for (const auto& elem : span)
		{
			if (!first)
			{
				*out++ = ' ';
			}
			first = false;
			out = std::format_to(out, "0x{:02x}", elem);
		}

		return out;
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
struct std::formatter<const std::vector<uint8_t>>
{
	constexpr auto parse(std::format_parse_context& ctx)
	{
		return ctx.begin();
	}

	template<typename Context>
	auto format(const std::vector<uint8_t>& vec, Context& ctx) const
	{
		auto out = ctx.out();
		bool first = true;

		for (const auto& elem : vec)
		{
			if (!first)
			{
				*out++ = ' ';
			}
			first = false;
			out = std::format_to(out, "0x{:02x}", elem);
		}

		return out;
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
