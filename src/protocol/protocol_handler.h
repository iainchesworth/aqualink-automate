#pragma once

#include <cstdint>
#include <expected>
#include <format>

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/use_awaitable.hpp>

#include "logging/logging.h"
#include "messages/message.h"
#include "messages/message_generator.h"
#include "messages/message_statemachine.h"
#include "protocol/protocol_errors.h"
#include "serial/serial_port.h"
#include "utility/array_standard_formatter.h"

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
		boost::asio::awaitable<std::expected<typename MESSAGE_GENERATOR::Message, ProtocolError>> HandleProtocol()
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
					LogDebug(Logging::Channel::Serial, std::format("Successfully yield'd -> async_read_some() ({} bytes read)", bytes_read));
					LogTrace(Logging::Channel::Serial, std::format("The following bytes were read from the serial device: {:.{}})", read_buffer, bytes_read));
					{
						if (auto message = m_MessageGenerator.GenerateMessageFromRawData(read_buffer, bytes_read); !message.has_value())
						{
							///TODO - NO MESSAGE, KEEP READING
						}
						else
						{
							co_return message.value();
						}
					}
					break;

				case boost::system::errc::operation_canceled:
				case boost::asio::error::operation_aborted:
					LogDebug(Logging::Channel::Serial, "Serial port's async_read_some() was cancelled.");
					co_return std::unexpected<ProtocolError>(ProtocolError::Failure);

				default:
					LogError(Logging::Channel::Serial, std::format("Error {} occured in protocol handler.  Error Code: {}", ec.to_string(), ec.value()));
					LogDebug(Logging::Channel::Serial, std::format("Error message: {}", ec.message()));
					co_return std::unexpected<ProtocolError>(ProtocolError::Failure);
				}
			
			} while (continue_processing);
		}

	private:
		AqualinkAutomate::Serial::SerialPort& m_SerialPort;
		MESSAGE_GENERATOR m_MessageGenerator;
	};
}
// namespace AqualinkAutomate::Protocol
