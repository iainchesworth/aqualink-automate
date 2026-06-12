#include "errors/message_errors.h"

namespace AqualinkAutomate::ErrorCodes
{
	const Message_ErrorCategory& Message_ErrorCategory::Instance()
	{
		static Message_ErrorCategory category;
		return category;
	}

	std::string_view Message_ErrorCategory::Describe(Message_ErrorCodes e)
	{
		switch (e)
		{
		case Message_ErrorCodes::Error_InvalidMessageData:
			return "The serial data did not form a valid message and could not be decoded";

		case Message_ErrorCodes::Error_CannotFindGenerator:
			return "No registered message generator recognised the serial data";

		case Message_ErrorCodes::Error_UnknownMessageType:
			return "The message type is not recognised by the factory";

		case Message_ErrorCodes::Error_GeneratorFailed:
			return "The message generator failed to produce a message from the serial data";

		case Message_ErrorCodes::Error_FailedToSerialize:
			return "The message could not be serialised to wire bytes";

		case Message_ErrorCodes::Error_FailedToDeserialize:
			return "The serial data could not be deserialised into the message";

		default:
			return "Unknown message error";
		}
	}
}
// namespace AqualinkAutomate::ErrorCodes

boost::system::error_code make_error_code(AqualinkAutomate::ErrorCodes::Message_ErrorCodes e)
{
	return AqualinkAutomate::ErrorCodes::Message_ErrorCategory::MakeErrorCode(e);
}

boost::system::error_condition make_error_condition(AqualinkAutomate::ErrorCodes::Message_ErrorCodes e)
{
	return AqualinkAutomate::ErrorCodes::Message_ErrorCategory::MakeErrorCondition(e);
}
