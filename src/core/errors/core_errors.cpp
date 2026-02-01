#include "errors/core_errors.h"

namespace AqualinkAutomate::ErrorCodes
{
	const Core_ErrorCategory& Core_ErrorCategory::Instance()
	{
		static Core_ErrorCategory category;
		return category;
	}

	const char* Core_ErrorCategory::name() const noexcept
	{
		return "AqualinkAutomate::Core Error Category";
	}

	std::string Core_ErrorCategory::message(int ev) const
	{
		switch (ev)
		{
		case Core_ErrorCodes::GeneralError:
			return "Core_ErrorCodes::GeneralError";

		case Core_ErrorCodes::UnknownCoreError:
			return "Core_ErrorCodes::UnknownCoreError";

		default:
			return "Core_ErrorCodes - Unknown Error Code";
		}
	}

}
// namespace AqualinkAutomate::ErrorCodes

boost::system::error_code make_error_code(const AqualinkAutomate::ErrorCodes::Core_ErrorCodes e)
{
	return boost::system::error_code(static_cast<int>(e), AqualinkAutomate::ErrorCodes::Core_ErrorCategory::Instance());
}

boost::system::error_condition make_error_condition(const AqualinkAutomate::ErrorCodes::Core_ErrorCodes e)
{
	return boost::system::error_condition(static_cast<int>(e), AqualinkAutomate::ErrorCodes::Core_ErrorCategory::Instance());
}
