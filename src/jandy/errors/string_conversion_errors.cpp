#include "jandy/errors/string_conversion_errors.h"

namespace AqualinkAutomate::ErrorCodes
{
	const StringConversion_ErrorCategory& StringConversion_ErrorCategory::Instance() {
		static StringConversion_ErrorCategory category;
		return category;
	}

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

boost::system::error_code make_error_code(const AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes e)
{
	return boost::system::error_code(static_cast<int>(e), AqualinkAutomate::ErrorCodes::StringConversion_ErrorCategory::Instance());
}

boost::system::error_condition make_error_condition(const AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes e)
{
	return boost::system::error_condition(static_cast<int>(e), AqualinkAutomate::ErrorCodes::StringConversion_ErrorCategory::Instance());
}