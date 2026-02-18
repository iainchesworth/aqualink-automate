#pragma once

#include <cctype>
#include <string>

namespace AqualinkAutomate::Utility
{

	/// Convert a label like "Filter Pump" to "filter_pump".
	/// Alphanumerics and underscores pass through (lowercased).
	/// Spaces, hyphens, and dots become underscores (collapsed).
	/// Other characters are stripped.
	inline std::string Slugify(const std::string& input)
	{
		std::string result;
		result.reserve(input.size());

		for (char c : input)
		{
			if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
			{
				result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			}
			else if (c == ' ' || c == '-' || c == '.')
			{
				if (!result.empty() && result.back() != '_')
				{
					result += '_';
				}
			}
		}

		if (!result.empty() && result.back() == '_')
		{
			result.pop_back();
		}

		return result;
	}

}
// namespace AqualinkAutomate::Utility
