#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include <boost/asio.hpp>
#include <boost/cobalt.hpp>

#include "interfaces/imessagesignal_recv.h"
#include "jandy/errors/jandy_errors_messages.h"
#include "jandy/errors/jandy_errors_protocol.h"
#include "protocol/protocol_handler.h"
#include "jandy/generator/jandy_message_generator.h"
#include "jandy/types/jandy_types.h"
#include "utility/array_standard_formatter.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Protocol
{
	boost::cobalt::generator<std::tuple<std::array<uint8_t, 16>, std::size_t>, std::reference_wrapper<Serial::SerialPort>> ProtocolHandler_ReadOp_GetSerialPortData()
	{
		Serial::SerialPort& serial_port = co_await boost::cobalt::this_coro::initial;

		while (!co_await boost::cobalt::this_coro::cancelled)
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("HandleRead -> Reading Serial Data", BOOST_CURRENT_LOCATION, Profiling::UnitColours::Red);

			std::array<uint8_t, 16> serial_data;

			auto [ec, bytes_read] = co_await serial_port.async_read_some(boost::asio::buffer(serial_data.data(), serial_data.size()), boost::asio::as_tuple(boost::cobalt::use_op));
			if (ec)
			{
				switch (ec.value())
				{
				case boost::asio::error::eof:
					LogDebug(Channel::Protocol, "Serial port's connection was closed by the peer...cannot continue.");
					break;

				case boost::asio::error::operation_aborted:
					LogDebug(Channel::Protocol, "Serial port's read_some() was cancelled or an error occurred.");
					break;

				default:
					LogDebug(Channel::Protocol, std::format("Error occurred in protocol handler (HandleRead).  Error Code: {}", ec.value()));
					LogDebug(Channel::Protocol, std::format("Error message: {}", ec.message()));
					break;
				}

				// Exit the co-routine...there's nothing else that can be done here...
				co_return std::make_tuple(std::array<uint8_t, 16>{}, 0);
			}
			else
			{
				LogDebug(Channel::Protocol, std::format("Successfully read data from the serial port; bytes received: {}", bytes_read));
				LogTrace(Channel::Protocol, std::format("The following bytes were read from the serial device: {:.{}})", std::span(serial_data.begin(), bytes_read), bytes_read));

				co_yield std::make_tuple(serial_data, bytes_read);
			}
		}

		// Exit the co-routine...there's nothing else that can be done here...
		co_return std::make_tuple(std::array<uint8_t, 16>{}, 0);
	}

	void ProtocolHandler_ReadOp_GetNextJandyMessage_MessageHandler(Types::JandyMessageTypePtr ptr)
	{
		LogTrace(Channel::Protocol, "Message received; emitting signal to listening consumer slots.");

		if (auto signal_ptr = std::dynamic_pointer_cast<Interfaces::IMessageSignalRecvBase>(ptr); nullptr != signal_ptr)
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("HandleRead -> Signalling Signals", BOOST_CURRENT_LOCATION);

			// Process the message...as per protocol requirements
			signal_ptr->Signal_MessageWasReceived();
		}
	}

	void ProtocolHandler_ReadOp_GetNextJandyMessage_ErrorHandler(Types::JandyErrorCode error_code)
	{
		switch (error_code.value())
		{
		case ErrorCodes::Message_ErrorCodes::Error_CannotFindGenerator:
			// This means there was a packet that could not be deserialised while processing....
			LogDebug(Logging::Channel::Protocol, "Protocol error while processing Jandy messages -> cannot find generator for message type");
			break;

		case ErrorCodes::Message_ErrorCodes::Error_GeneratorFailed:
			// This means there was a packet that could not be deserialised while processing....
			LogDebug(Logging::Channel::Protocol, "Protocol error while processing Jandy messages -> generator failed to create message");
			break;

		case ErrorCodes::Message_ErrorCodes::Error_FailedToDeserialize:
			// This means there was a packet that could not be deserialised while processing....
			LogDebug(Logging::Channel::Protocol, "Protocol error while processing Jandy messages -> failed to deserialize bytes");
			break;

		case ErrorCodes::Protocol_ErrorCodes::DataAvailableToProcess:
			[[fallthrough]];
		case ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData:
			[[fallthrough]];
		default:
			LogDebug(Logging::Channel::Protocol, std::format("Protocol error while processing Jandy messages -> unknown error occured: {}, {}", error_code.value(), error_code.message()));
			break;
		}
	}

	boost::cobalt::promise<void> ProtocolHandler_ReadOp(Serial::SerialPort& serial_port)
	{
		auto profiling_domain = Factory::ProfilingUnitFactory::Instance().CreateDomain("ProtocolHandler");
		auto profiling_frame = Factory::ProfilingUnitFactory::Instance().CreateFrame("ProtocolHandler -> Frame");

		profiling_domain->Start();

		auto serial_port_reader = ProtocolHandler_ReadOp_GetSerialPortData();

		std::vector<uint8_t> serial_data_buffer;

		while (!co_await boost::cobalt::this_coro::cancelled)
		{
			profiling_frame->Start();

			auto [serial_data, bytes_read] = co_await serial_port_reader(serial_port);

			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("HandleRead -> Read Success; Processing Serial Data", BOOST_CURRENT_LOCATION);

			std::copy(serial_data.begin(), serial_data.begin() + bytes_read, std::back_inserter(serial_data_buffer));

			Generators::GenerateMessageFromRawData(serial_data_buffer)
				.map(ProtocolHandler_ReadOp_GetNextJandyMessage_MessageHandler)
				.or_else(ProtocolHandler_ReadOp_GetNextJandyMessage_ErrorHandler);

			profiling_frame->End();
		}

		profiling_domain->End();
	}

}
// namespace AqualinkAutomate::Protocol
