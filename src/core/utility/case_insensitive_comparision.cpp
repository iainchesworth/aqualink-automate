#include <cctype>

#include "utility/case_insensitive_comparision.h"

namespace AqualinkAutomate::Utility
{

	bool CaseInsensitiveComparison(const unsigned char lhs, const unsigned char rhs)
	{
		return std::tolower(lhs) == std::tolower(rhs);
	}

}
// namespace AqualinkAutomate::Utility
