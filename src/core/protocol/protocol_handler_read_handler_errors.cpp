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

	void ProtocolHandler_ReadOp_ErrorHandler(ProtocolErrorCode error_code, std::shared_ptr<Kernel::StatisticsHub> statistics_hub)
	{
		switch (error_code.value())
		{
		case ErrorCodes::Message_ErrorCodes::Error_CannotFindGenerator:
			LogDebug(Channel::Protocol, "Protocol error while processing messages -> cannot find generator for message type");
			if (statistics_hub) { ++statistics_hub->MessageErrors.GeneratorFailures; }
			break;

		case ErrorCodes::Message_ErrorCodes::Error_GeneratorFailed:
			LogDebug(Channel::Protocol, "Protocol error while processing messages -> generator failed to create message");
			if (statistics_hub) { ++statistics_hub->MessageErrors.GeneratorFailures; }
			break;

		case ErrorCodes::Message_ErrorCodes::Error_FailedToDeserialize:
			LogDebug(Channel::Protocol, "Protocol error while processing messages -> failed to deserialize bytes");
			if (statistics_hub) { ++statistics_hub->MessageErrors.DeserializationFailures; }
			break;

		case ErrorCodes::Protocol_ErrorCodes::InvalidPacketFormat:
			LogDebug(Channel::Protocol, "Protocol error while processing messages -> invalid packet format");
			if (statistics_hub) { ++statistics_hub->MessageErrors.InvalidPacketFormat; }
			break;

		case ErrorCodes::Protocol_ErrorCodes::ChecksumFailure:
			LogDebug(Channel::Protocol, "Protocol error while processing messages -> checksum failure");
			if (statistics_hub) { ++statistics_hub->MessageErrors.ChecksumFailures; }
			break;

		case ErrorCodes::Protocol_ErrorCodes::OverlappingPackets:
			LogDebug(Channel::Protocol, "Protocol error while processing messages -> overlapping packets detected");
			if (statistics_hub) { ++statistics_hub->MessageErrors.OverlappingPackets; }
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

