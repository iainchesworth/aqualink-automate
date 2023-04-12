#pragma once

#include <type_traits>

namespace AqualinkAutomate::ErrorCodes
{

	struct ErrorCode 
	{
		bool operator==(const ErrorCode& other) const;
	};

}
// namespace AqualinkAutomate::ErrorCodes

