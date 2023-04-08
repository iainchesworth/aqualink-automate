#pragma once

#include <algorithm>
#include <cstdint>
#include <format>
#include <string>

template<>
struct std::formatter<std::array<uint8_t, 16>> : std::formatter<std::string>
{
	template<typename Context>
	auto format(const std::array<uint8_t, 16>& arr, Context& context) const
	{
		std::for_each_n(arr.begin(), 16, [&context](auto& elem)
			{
				context.out() = elem;
			}
		);

		return context.out();
	}
};

namespace AqualinkAutomate::Utility
{

}
// using namespace AqualinkAutomate::Utility
