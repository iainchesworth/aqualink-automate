#include <array>
#include <format>

#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/system/error_code.hpp>

#include "interfaces/imessagesignal_recv.h"
#include "jandy/errors/jandy_errors_messages.h"
#include "jandy/errors/jandy_errors_protocol.h"
#include "jandy/generator/jandy_message_generator.h"
#include "logging/logging.h"
#include "profiling/profiling.h"
#include "protocol/protocol_handler.h"
#include "utility/array_standard_formatter.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Protocol
{

	bool ProtocolHandler::HandleRead()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("HandleRead -> Reading Serial Data", std::source_location::current(), Profiling::UnitColours::Red);

		std::lock_guard<std::mutex> lock(m_SerialData_IncomingMutex);

		std::array<Interfaces::ISerialPort::DataType, 16> read_buffer;
		bool continue_processing = false;
		boost::system::error_code ec;
		std::size_t bytes_read;

		bytes_read = m_SerialPort.read_some(boost::asio::buffer(read_buffer, read_buffer.size()), ec);
		switch (ec.value())
		{
		case boost::system::errc::success:
			LogDebug(Channel::Protocol, std::format("Successfully read data from the serial port; bytes received: {}", bytes_read));
			continue_processing = HandleRead_Success(read_buffer, bytes_read);
			break;

		case boost::asio::error::eof:
			LogDebug(Channel::Protocol, "Serial port's connection was closed by the peer...cannot continue.");
			break;

		case boost::system::errc::operation_canceled:
		case boost::asio::error::operation_aborted:
			LogDebug(Channel::Protocol, "Serial port's async_read_some() was cancelled or an error occurred.");
			break;

		default:
			LogError(Channel::Protocol, std::format("Error {} occured in protocol handler.  Error Code: {}", ec.to_string(), ec.value()));
			LogDebug(Channel::Protocol, std::format("Error message: {}", ec.message()));
			break;
		}

		return continue_processing;
	}

	bool ProtocolHandler::HandleRead_Success(auto& read_buffer, auto bytes_read)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("HandleRead -> Read Success; Processing Serial Data", std::source_location::current());
		
		auto read_buffer_span = std::span(read_buffer.begin(), bytes_read);
		
		LogTrace(Channel::Protocol, std::format("The following bytes were read from the serial device: {:.{}})", read_buffer_span, read_buffer_span.size()));

		std::move(read_buffer.begin(), read_buffer.begin() + bytes_read, std::back_inserter(m_SerialData_Incoming));

		// Starting processing the message generator.
		bool continue_processing = true;
		bool process_packets = true;

		do
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("HandleRead -> Generating Messages", std::source_location::current());

			auto message = Generators::GenerateMessageFromRawData(m_SerialData_Incoming);
			if (message.has_value())
			{
				LogTrace(Channel::Protocol, "Message received; emitting signal to listening consumer slots.");

				if (auto signal_ptr = std::dynamic_pointer_cast<Interfaces::IMessageSignalRecvBase>(message.value()); nullptr != signal_ptr)
				{
					auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("HandleRead -> Signalling Signals", std::source_location::current());

					// Process the message...as per protocol requirements
					signal_ptr->Signal_MessageWasReceived();
				}
			}
			else
			{
				LogDebug(Logging::Channel::Protocol, "Protocol error while processing Jandy messages.");

				switch (boost::system::error_code(message.error()).value())
				{
				case ErrorCodes::Protocol_ErrorCodes::DataAvailableToProcess:
					// Continue processing data looking for messages in the buffer...
					break;

				case ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData:
					process_packets = false;
					break;

				case ErrorCodes::Message_ErrorCodes::Error_CannotFindGenerator:
					// This means there was a packet that could not be deserialised while processing....
					break;

				case ErrorCodes::Message_ErrorCodes::Error_GeneratorFailed:
					// This means there was a packet that could not be deserialised while processing....
					break;

				case ErrorCodes::Message_ErrorCodes::Error_FailedToDeserialize:
					// This means there was a packet that could not be deserialised while processing....
					break;

				default:
					continue_processing = false;
					process_packets = false;
					break;
				}
			}

		} while (process_packets);

		return continue_processing;
	}

}
// namespace AqualinkAutomate::Protocol