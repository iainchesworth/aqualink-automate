#pragma once

#include <array>

#include <boost/describe.hpp>

namespace AqualinkAutomate::Utility
{

	template<class E> struct enum_descriptor
	{
		E value;
		char const* name;
	};

	template<class E, template<class... T> class L, class... T>
	constexpr std::array<enum_descriptor<E>, sizeof...(T)> describe_enumerators_as_array_impl(L<T...>)
	{
		return { { { T::value, T::name }... } };
	}

	template<class E> constexpr auto describe_enumerators_as_array()
	{
		return describe_enumerators_as_array_impl<E>(boost::describe::describe_enumerators<E>());
	}

}
// namespace AqualinkAutomate::Utility
