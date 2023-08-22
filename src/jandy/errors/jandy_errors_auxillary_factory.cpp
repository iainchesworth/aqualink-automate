#include "jandy/errors/jandy_errors_auxillary_factory.h"

namespace AqualinkAutomate::ErrorCodes
{
	const Factory_ErrorCategory& Factory_ErrorCategory::Instance()
	{
		static Factory_ErrorCategory category;
		return category;
	}

	const char* Factory_ErrorCategory::name() const noexcept
	{
		return "AqualinkAutomate::Factory Error Category";
	}

	std::string Factory_ErrorCategory::message(int ev) const
	{
		switch (ev)
		{
		case Factory_ErrorCodes::Error_UnknownFactoryError:
			return "Factory_ErrorCodes::Error_UnknownFactoryError";

		case Factory_ErrorCodes::Error_UnknownDeviceLabel:
			return "Factory_ErrorCodes::Error_UnknownDeviceLabel";

		case Factory_ErrorCodes::Error_CannotCastToJandyAuxillaryId:
			return "Factory_ErrorCodes::Error_CannotCastToJandyAuxillaryId";

		case Factory_ErrorCodes::Error_FailedToCreateAuxillaryPtr:
			return "Factory_ErrorCodes::Error_FailedToCreateAuxillaryPtr";

		case Factory_ErrorCodes::Error_ReceivedInvalidAuxillaryStatus:
			return "Factory_ErrorCodes::Error_ReceivedInvalidAuxillaryStatus";

		default:
			return "Factory_ErrorCodes - Unknown Error Code";
		}
	}
}
// namespace AqualinkAutomate::ErrorCodes

boost::system::error_code make_error_code(AqualinkAutomate::ErrorCodes::Factory_ErrorCodes e)
{
	return boost::system::error_code(static_cast<int>(e), AqualinkAutomate::ErrorCodes::Factory_ErrorCategory::Instance());
}

boost::system::error_condition make_error_condition(const AqualinkAutomate::ErrorCodes::Factory_ErrorCodes e)
{
	return boost::system::error_condition(static_cast<int>(e), AqualinkAutomate::ErrorCodes::Factory_ErrorCategory::Instance());
}
