#pragma once

#include <concepts>
#include <type_traits>

namespace AqualinkAutomate::Concepts
{

	template<typename T, typename... Types>
	concept IsOneOf = std::disjunction_v<std::is_same<T, Types>...>;

}
// namespace AqualinkAutomate::Concepts
