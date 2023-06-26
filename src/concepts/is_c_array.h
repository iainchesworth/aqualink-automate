#pragma once

#include <concepts>

namespace AqualinkAutomate::Concepts
{

	template <typename T, std::size_t N>
	constexpr std::true_type is_c_array_impl(const T(&)[N]) { return {}; }

	template <typename T>
	constexpr std::false_type is_c_array_impl(const T&) { return {}; }

	template <typename T>
	constexpr auto is_c_array() -> decltype(is_c_array_impl(std::declval<const T&>())) { return {}; }

	template <typename T>
	concept CArrayRef = (bool)is_c_array<const T&>() && std::is_reference_v<T>;

}
// namespace AqualinkAutomate::Concepts
