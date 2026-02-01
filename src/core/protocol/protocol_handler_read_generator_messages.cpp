#include <array>
#include <cstddef>
#include <memory>
#include <tuple>
#include <vector>

#include "errors/protocol_errors.h"
#include "logging/logging.h"
#include "profiling/profiling.h"
#include "protocol/message_generator_registry.h"
#include "protocol/protocol_handler_read.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Protocol
{

	boost::cobalt::generator<ReadOps_MessageType, ReadOps_SerialBufferRefType> ProtocolHandler_ReadOp_GetMessagesFromSerialData()
	{
		ReadOps_SerialBufferType& serial_circular_data_buffer = co_await boost::cobalt::this_coro::initial;

		LogTrace(Channel::Protocol, "START: Protocol::ProtocolHandler_ReadOp_GetMessagesFromSerialData");

		while (!co_await boost::cobalt::this_coro::cancelled)
		{
			if (serial_circular_data_buffer.empty())
			{
				co_yield std::unexpected(make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData));
			}

			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("HandleRead -> Processing Message from Buffer", std::source_location::current());

			// Use the message generator registry to try all registered protocol generators
			co_yield MessageGeneratorRegistry::Instance().GenerateMessage(serial_circular_data_buffer);
		}

		co_return std::unexpected(make_error_code(ErrorCodes::Protocol_ErrorCodes::UnknownFailure));
	}

}
// namespace AqualinkAutomate::Protocol

