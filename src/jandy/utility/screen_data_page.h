#pragma once

#include <array>
#include <string>

namespace AqualinkAutomate::Utility
{

	template<std::size_t ROWS>
	using ScreenDataPage = std::array<std::string, ROWS>;

}
// namespace AqualinkAutomate::Utility
