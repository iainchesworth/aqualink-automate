#pragma once

#include <cstdint>
#include <expected>
#include <format>
#include <span>

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/use_awaitable.hpp>

#include "errors/error_codes.h"
#include "errors/protocol_error_codes.h"
#include "logging/logging.h"
#include "messages/message.h"
#include "messages/message_generator.h"
#include "serial/serial_port.h"
#include "utility/array_standard_formatter.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::ErrorCodes;

namespace AqualinkAutomate::Protocol
{
	template<typename MESSAGE_GENERATOR>
	class ProtocolHandler
	{
	public:
		ProtocolHandler(AqualinkAutomate::Serial::SerialPort& serial_port) :
			m_SerialPort(serial_port),
			m_MessageGenerator()
		{
		}

	public:
		boost::asio::awaitable<std::expected<typename MESSAGE_GENERATOR::MessageType, boost::system::error_code>> HandleProtocol()
		{
			std::array<uint8_t, 16> read_buffer;
			bool continue_processing = true;

			do
			{ 
				auto buffer = boost::asio::buffer(read_buffer, read_buffer.size());
				auto [ec, bytes_read] = co_await m_SerialPort.async_read_some(buffer, boost::asio::as_tuple(boost::asio::use_awaitable));
				switch (ec.value())
				{
				case boost::system::errc::success:
					{
						auto read_buffer_span = std::span(read_buffer.begin(), bytes_read);
						LogTrace(Logging::Channel::Serial, std::format("The following bytes were read from the serial device: {:.{}})", read_buffer_span, read_buffer_span.size()));
						LogTrace(Logging::Channel::Serial, std::format("Successfully yield'd -> async_read_some() ({} bytes read)", bytes_read));

						m_MessageGenerator.InjectRawSerialData(read_buffer_span);
						bool process_packets = true;

						do
						{
							auto message = co_await m_MessageGenerator.GenerateMessageFromRawData();
							if (message.has_value())
							{
								// Process the message...as per protocol requirements
								///TODO -> message.value();
							}
							else if (message.error() == make_error_code(ErrorCodes::Protocol_ErrorCodes::DataAvailableToProcess))
							{
								// Continue processing data looking for messages in the buffer...
							}
							else if (message.error() == make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData))
							{
								process_packets = false;
							}
							else
							{
								LogDebug(Logging::Channel::Serial, "Protocol error while processing Jandy messages.");
								continue_processing = false;
								process_packets = false;
							}

						} while (process_packets);
					}
					break;

				case boost::asio::error::eof:
					LogDebug(Logging::Channel::Serial, "Serial port's connection was closed by the peer...cannot continue.");
					continue_processing = false;
					break;

				case boost::system::errc::operation_canceled:
				case boost::asio::error::operation_aborted:
					LogDebug(Logging::Channel::Serial, "Serial port's async_read_some() was cancelled or an error occurred.");
					continue_processing = false;
					break;

				default:
					LogError(Logging::Channel::Serial, std::format("Error {} occured in protocol handler.  Error Code: {}", ec.to_string(), ec.value()));
					LogDebug(Logging::Channel::Serial, std::format("Error message: {}", ec.message()));
					continue_processing = false;
					break;
				}
			
			} while (continue_processing);

			co_return std::unexpected<boost::system::error_code>(make_error_code(ErrorCodes::Protocol_ErrorCodes::UnknownFailure));
		}

	private:
		AqualinkAutomate::Serial::SerialPort& m_SerialPort;
		MESSAGE_GENERATOR m_MessageGenerator;
	};
}
// namespace AqualinkAutomate::Protocol
