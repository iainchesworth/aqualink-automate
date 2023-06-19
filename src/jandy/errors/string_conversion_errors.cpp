#include "jandy/errors/string_conversion_errors.h"

namespace AqualinkAutomate::ErrorCodes
{

	const char* StringConversion_ErrorCategory::name() const noexcept
	{
		return "AqualinkAutomate::String Conversion Error Category";
	}

	std::string StringConversion_ErrorCategory::message(int ev) const
	{
		switch (ev)
		{
		case StringConversion_ErrorCodes::MalformedInput:
			return "StringConversion_ErrorCodes::MalformedInput";

		case StringConversion_ErrorCodes::UnknownStringConversionFailure:
			return "StringConversion_ErrorCodes::UnknownStringConversionFailure";

		default:
			return "StringConversion_ErrorCodes - Unknown Error Code";
		}
	}

}
// namespace AqualinkAutomate::ErrorCodes

boost::system::error_code make_error_code(AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes e)
{
	static AqualinkAutomate::ErrorCodes::StringConversion_ErrorCategory category;
	return boost::system::error_code(static_cast<int>(e), category);
}
