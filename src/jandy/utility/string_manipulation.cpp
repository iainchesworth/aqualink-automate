#include <algorithm>
#include <cctype>

#include "utility/string_manipulation.h"

namespace AqualinkAutomate::Utility
{

	std::string TrimWhitespace(const std::string& string)
	{
		auto start = std::find_if_not(string.begin(), string.end(), [](unsigned char c) { return std::isspace(c); });
		auto end = std::find_if_not(string.rbegin(), string.rend(), [](unsigned char c) { return std::isspace(c); }).base();

		return (start >= end ? std::string("") : std::string(start, end));
	}

}
// namespace AqualinkAutomate::Utility
