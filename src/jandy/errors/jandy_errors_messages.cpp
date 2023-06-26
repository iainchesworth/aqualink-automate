#include "jandy/errors/jandy_errors_messages.h"

namespace AqualinkAutomate::ErrorCodes
{
	const Message_ErrorCategory& Message_ErrorCategory::Instance() {
		static Message_ErrorCategory category;
		return category;
	}

	const char* Message_ErrorCategory::name() const noexcept
	{
		return "AqualinkAutomate::Message Error Category";
	}

	std::string Message_ErrorCategory::message(int ev) const
	{
		switch (ev)
		{
		case Message_ErrorCodes::Error_InvalidMessageData:
			return "Message_ErrorCodes::Error_InvalidMessageData";

		case Message_ErrorCodes::Error_UnknownMessageType:
			return "Message_ErrorCodes::Error_UnknownMessageType";

		default:
			return "Message_ErrorCodes - Unknown Error Code";
		}
	}
}
// namespace AqualinkAutomate::ErrorCodes

boost::system::error_code make_error_code(AqualinkAutomate::ErrorCodes::Message_ErrorCodes e)
{
	return boost::system::error_code(static_cast<int>(e), AqualinkAutomate::ErrorCodes::Message_ErrorCategory::Instance());
}

boost::system::error_condition make_error_condition(const AqualinkAutomate::ErrorCodes::Message_ErrorCodes e)
{
	return boost::system::error_condition(static_cast<int>(e), AqualinkAutomate::ErrorCodes::Message_ErrorCategory::Instance());
}
