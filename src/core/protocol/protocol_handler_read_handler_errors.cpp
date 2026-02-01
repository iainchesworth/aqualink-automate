#include <format>

#include "errors/message_errors.h"
#include "errors/protocol_errors.h"
#include "logging/logging.h"
#include "profiling/profiling.h"
#include "protocol/protocol_handler_read.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Protocol
{

	void ProtocolHandler_ReadOp_ErrorHandler(ProtocolErrorCode error_code)
	{
		switch (error_code.value())
		{
		case ErrorCodes::Message_ErrorCodes::Error_CannotFindGenerator:
			// This means there was a packet that could not be deserialised while processing....
			LogDebug(Channel::Protocol, "Protocol error while processing messages -> cannot find generator for message type");
			break;

		case ErrorCodes::Message_ErrorCodes::Error_GeneratorFailed:
			// This means there was a packet that could not be deserialised while processing....
			LogDebug(Channel::Protocol, "Protocol error while processing messages -> generator failed to create message");
			break;

		case ErrorCodes::Message_ErrorCodes::Error_FailedToDeserialize:
			// This means there was a packet that could not be deserialised while processing....
			LogDebug(Channel::Protocol, "Protocol error while processing messages -> failed to deserialize bytes");
			break;

		case ErrorCodes::Protocol_ErrorCodes::DataAvailableToProcess:
			[[fallthrough]];
		case ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData:
			LogTrace(Channel::Protocol, std::format("Protocol processing paused: {}", error_code.message()));
			break;

		default:
			LogDebug(Channel::Protocol, std::format("Protocol error while processing messages -> unknown error occured: {}, {}", error_code.value(), error_code.message()));
			break;
		}
	}

}
// namespace AqualinkAutomate::Protocol

