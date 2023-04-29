#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <utility>

namespace AqualinkAutomate::Utility
{

	template<std::size_t ROWS>
	using ScreenDataPage = std::array<std::string, ROWS>;

}
// namespace AqualinkAutomate::Utility
