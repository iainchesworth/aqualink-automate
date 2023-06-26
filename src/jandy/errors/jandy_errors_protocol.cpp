#include "jandy/errors/jandy_errors_protocol.h"

namespace AqualinkAutomate::ErrorCodes
{
	const Protocol_ErrorCategory& Protocol_ErrorCategory::Instance() {
		static Protocol_ErrorCategory category;
		return category;
	}

	const char* Protocol_ErrorCategory::name() const noexcept
	{
		return "AqualinkAutomate::Protocol Error Category";
	}

	std::string Protocol_ErrorCategory::message(int ev) const
	{
		switch (ev)
		{
		case Protocol_ErrorCodes::DataAvailableToProcess:
			return "Protocol_ErrorCodes::DataAvailableToProcess";

		case Protocol_ErrorCodes::WaitingForMoreData:
			return "Protocol_ErrorCodes::WaitingForMoreData";

		case Protocol_ErrorCodes::InvalidPacketFormat:
			return "Protocol_ErrorCodes::InvalidPacketFormat";

		case Protocol_ErrorCodes::UnknownFailure:
			return "Protocol_ErrorCodes::UnknownFailure";

		default:
			return "Protocol_ErrorCodes - Unknown Error Code";
		}
	}

}
// namespace AqualinkAutomate::ErrorCodes

boost::system::error_code make_error_code(AqualinkAutomate::ErrorCodes::Protocol_ErrorCodes e)
{
	return boost::system::error_code(static_cast<int>(e), AqualinkAutomate::ErrorCodes::Protocol_ErrorCategory::Instance());
}

boost::system::error_condition make_error_condition(const AqualinkAutomate::ErrorCodes::Protocol_ErrorCodes e)
{
	return boost::system::error_condition(static_cast<int>(e), AqualinkAutomate::ErrorCodes::Protocol_ErrorCategory::Instance());
}
