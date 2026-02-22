#pragma once

#include <charconv>
#include <concepts>
#include <format>
#include <optional>
#include <string_view>
#include <typeinfo>
#include <type_traits>

#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{

	template<typename T>
	concept IntegralType = std::is_integral_v<T>;

	template<IntegralType NUMERIC_TYPE>
	auto ToNumber(const std::string_view str) -> std::optional<NUMERIC_TYPE>
	{
		NUMERIC_TYPE value;

		if (auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value); std::errc() == ec && ptr == str.data() + str.size())
		{
			return value;
		}
		else if (std::errc::invalid_argument == ec)
		{
			LogDebug(Channel::Developer, std::format("No valid pattern match while attempting to convert {} to a number", str));
		}
		else if (std::errc::result_out_of_range == ec)
		{
			LogDebug(Channel::Developer, std::format("Cannot represent {} as a number in provided type: {}", str, typeid(value).name()));
		}
		else 
		{
			LogDebug(Channel::Developer, std::format("Unknown error attempting to convert {} into a number", str));
		}

		return std::nullopt;
	}

}
// namespace AqualinkAutomate::Utility
