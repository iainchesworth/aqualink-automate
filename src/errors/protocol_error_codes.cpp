#include "errors/protocol_error_codes.h"

namespace AqualinkAutomate::ErrorCodes
{
	
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
	static AqualinkAutomate::ErrorCodes::Protocol_ErrorCategory category;
	return boost::system::error_code(static_cast<int>(e), category);
}
