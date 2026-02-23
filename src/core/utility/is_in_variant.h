#pragma once 

#include <type_traits>
#include <variant>

namespace AqualinkAutomate::Utility
{

	template<class T, class Variant> 
	struct IsInVariant : std::false_type {};

	template<class T, class... Alts>
	struct IsInVariant<T, std::variant<Alts...>> : std::bool_constant<(std::is_same_v<T, Alts> || ...)> {};

	template<class T, class Variant>
	inline constexpr bool IsInVariantV = IsInVariant<T, Variant>::value;

}
// namespace AqualinkAutomate::Utility
