#pragma once

#include <type_traits>

namespace AqualinkAutomate::Concepts
{

	// Satisfied when T is (or is a reference to) a C-style bounded array - e.g.
	// int[5], const char[N], or const char(&)[N] (the form a `const auto&` NTTP
	// binds to). Built directly on std::is_bounded_array_v rather than the previous
	// overload-resolution SFINAE helpers; reference and cv-qualifiers are stripped
	// first so it accepts the same set of types as the original concept.
	template<typename T>
	concept CArray = std::is_bounded_array_v<std::remove_cvref_t<T>>;

	// Satisfied when T is a reference to a C-style bounded array (e.g. int(&)[5]).
	template<typename T>
	concept CArrayRef = std::is_reference_v<T> && std::is_bounded_array_v<std::remove_reference_t<T>>;

}
// namespace AqualinkAutomate::Concepts
