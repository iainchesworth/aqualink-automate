#include "errors/core_errors.h"

namespace AqualinkAutomate::ErrorCodes
{
	const Options_ErrorCategory& Options_ErrorCategory::Instance()
	{
		static Options_ErrorCategory category;
		return category;
	}

	const char* Options_ErrorCategory::name() const noexcept
	{
		return "AqualinkAutomate::Options Error Category";
	}

	std::string Options_ErrorCategory::message(int ev) const
	{
		switch (ev)
		{
		case Options_ErrorCodes::OptionsAreaAlreadyRegistered:
			return "Options_ErrorCodes::OptionsAreaAlreadyRegistered";

		case Options_ErrorCodes::OptionsValidationFailed:
			return "Options_ErrorCodes::OptionsValidationFailed";

		case Options_ErrorCodes::OptionsParsingFailed:
			return "Options_ErrorCodes::OptionsParsingFailed";

		case Options_ErrorCodes::OptionsHandlingFailed:
			return "Options_ErrorCodes::OptionsHandlingFailed";

		case Options_ErrorCodes::UnknownOptionsError:
			return "Options_ErrorCodes::UnknownOptionsError";

		default:
			return "Options_ErrorCodes - Unknown Error Code";
		}
	}

}
// namespace AqualinkAutomate::ErrorCodes

boost::system::error_code make_error_code(const AqualinkAutomate::ErrorCodes::Options_ErrorCodes e)
{
	return boost::system::error_code(static_cast<int>(e), AqualinkAutomate::ErrorCodes::Options_ErrorCategory::Instance());
}

boost::system::error_condition make_error_condition(const AqualinkAutomate::ErrorCodes::Options_ErrorCodes e)
{
	return boost::system::error_condition(static_cast<int>(e), AqualinkAutomate::ErrorCodes::Options_ErrorCategory::Instance());
}
