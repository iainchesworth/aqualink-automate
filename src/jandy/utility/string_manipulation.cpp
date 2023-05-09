#include <algorithm>

#include "jandy/utility/string_manipulation.h"

namespace AqualinkAutomate::Utility
{

	std::string TrimWhitespace(const std::string& string)
	{
		auto start = std::find_if_not(string.begin(), string.end(), [](int c) { return std::isspace(c); });
		auto end = std::find_if_not(string.rbegin(), string.rend(), [](int c) { return std::isspace(c); }).base();

		return (start >= end ? std::string("") : std::string(start, end));
	}

}
// namespace AqualinkAutomate::Utility
