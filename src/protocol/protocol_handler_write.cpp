#include <format>
#include <mutex>

#include <boost/asio.hpp>
#include <boost/cobalt.hpp>

#include "jandy/generator/jandy_message_generator.h"
#include "logging/logging.h"
#include "profiling/profiling.h"
#include "protocol/protocol_handler.h"
#include "utility/array_standard_formatter.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Protocol
{

	namespace
	{
		std::mutex ProtocolHandler_WriteOp_MessagePublisher_Mutex;
	}
	// namespace

	boost::cobalt::promise<void> ProtocolHandler_WriteOp_MessagePublisher(Serial::SerialPort& serial_port, std::vector<uint8_t> buffer)
	{
		std::lock_guard<std::mutex> guard(ProtocolHandler_WriteOp_MessagePublisher_Mutex);

		while (!co_await boost::cobalt::this_coro::cancelled)
		{
			auto [ec, bytes_written] = co_await serial_port.async_write_some(boost::asio::buffer(buffer), boost::asio::as_tuple(boost::cobalt::use_op));
			if (ec)
			{
				switch (ec.value())
				{
				case boost::asio::error::eof:
					LogDebug(Channel::Protocol, "Serial port's connection was closed by the peer...cannot continue.");
					break;

				case boost::asio::error::operation_aborted:
					LogDebug(Channel::Protocol, "Serial port's async_write_some() was cancelled or an error occurred.");
					break;

				default:
					LogDebug(Channel::Protocol, std::format("Error occurred in protocol handler (HandleWrite).  Error Code: {}", ec.value()));
					LogDebug(Channel::Protocol, std::format("Error message: {}", ec.message()));
					break;
				}

				// Exit this co-routine as there's nothing more to be done.
				break;
			}
			else if (bytes_written == buffer.size())
			{
				LogTrace(Channel::Protocol, "Completed all serial data writing; clearing internal write buffer");

				// Exit this co-routine as there's nothing more to be done.
				break;
			}
			else
			{
				LogTrace(Channel::Protocol, std::format("Only partially completed serial data writing; {} bytes of {} bytes were transmitted", bytes_written, buffer.size()));
				buffer.erase(buffer.begin(), buffer.begin() + bytes_written);
			}
		}

		co_return;
	}

}
// namespace AqualinkAutomate::Protocol
