#include "errors/string_conversion_errors.h"

namespace AqualinkAutomate::ErrorCodes
{
	const StringConversion_ErrorCategory& StringConversion_ErrorCategory::Instance()
	{
		static StringConversion_ErrorCategory category;
		return category;
	}

	std::string_view StringConversion_ErrorCategory::Describe(StringConversion_ErrorCodes e)
	{
		switch (e)
		{
		case StringConversion_ErrorCodes::MalformedInput:
			return "The input string was malformed and could not be converted";

		case StringConversion_ErrorCodes::UnknownStringConversionFailure:
			return "An unspecified string conversion failure occurred";

		default:
			return "Unknown string conversion error";
		}
	}

}
// namespace AqualinkAutomate::ErrorCodes

boost::system::error_code make_error_code(AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes e)
{
	return AqualinkAutomate::ErrorCodes::StringConversion_ErrorCategory::MakeErrorCode(e);
}

boost::system::error_condition make_error_condition(AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes e)
{
	return AqualinkAutomate::ErrorCodes::StringConversion_ErrorCategory::MakeErrorCondition(e);
}
