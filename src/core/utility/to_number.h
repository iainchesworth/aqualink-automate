#pragma once

#include <charconv>
#include <concepts>
#include <format>
#include <optional>
#include <string_view>
#include <system_error>

#include "logging/logging.h"

namespace AqualinkAutomate::Utility
{

	// A Number is any type std::from_chars can parse: every integral type plus every
	// floating-point type. (std::integral already excludes bool/char where unwanted at
	// the call site, and std::floating_point adds the from_chars float path.)
	template<typename T>
	concept Number = std::integral<T> || std::floating_point<T>;

	template<Number NUMERIC_TYPE>
	auto ToNumber(const std::string_view str) -> std::optional<NUMERIC_TYPE>
	{
		NUMERIC_TYPE value{};

		if (auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value); std::errc() == ec && ptr == str.data() + str.size())
		{
			return value;
		}
		else if (std::errc::invalid_argument == ec)
		{
			LogDebug(Logging::Channel::Developer, std::format("No valid pattern match while attempting to convert {} to a number", str));
		}
		else if (std::errc::result_out_of_range == ec)
		{
			LogDebug(Logging::Channel::Developer, std::format("Cannot represent {} as a number in the requested numeric type", str));
		}
		else
		{
			LogDebug(Logging::Channel::Developer, std::format("Unknown error attempting to convert {} into a number", str));
		}

		return std::nullopt;
	}

}
// namespace AqualinkAutomate::Utility
