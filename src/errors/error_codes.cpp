#include "errors/error_codes.h"

namespace AqualinkAutomate::ErrorCodes
{

	bool ErrorCode::operator==(const ErrorCode& other) const
	{
		return std::is_same<decltype(*this), decltype(other)>::value;
	}

}
// namespace AqualinkAutomate::ErrorCodes
