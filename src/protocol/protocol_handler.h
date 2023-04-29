#pragma once

#include <cstdint>
#include <expected>
#include <format>
#include <memory>
#include <span>
#include <vector>

#include <chrono>
#include <thread>

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_awaitable.hpp>

#include "interfaces/igenerator.h"
#include "interfaces/imessage.h"
#include "interfaces/imessagesignal.h"
#include "interfaces/iserialport.h"
#include "jandy/errors/jandy_errors_messages.h"
#include "jandy/errors/jandy_errors_protocol.h"
#include "logging/logging.h"
#include "profiling/profiling.h"
#include "serial/serial_port.h"
#include "utility/array_standard_formatter.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::ErrorCodes;
using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Protocol
{
	template<typename GENERATOR_TYPE>
	class ProtocolHandler
	{
	public:
		ProtocolHandler(boost::asio::io_context& io_context, Serial::SerialPort& serial_port, GENERATOR_TYPE& generator) :
			m_ProfilingDomain(),
			m_SerialPort(serial_port),
			m_Generator(generator)
		{
			static_cast<void>(Factory::ProfilerFactory::Instance().Get()->CreateDomain("ProtocolHandler"));
		}

		~ProtocolHandler()
		{
		}

	public:
		boost::asio::awaitable<void> Run()
		{
			auto pu_frame = Factory::ProfilingUnitFactory::Instance().CreateFrame("ProtocolHandler::Run");

			bool continue_processing = true;

			do
			{
				pu_frame->Start();

				continue_processing = co_await HandleProtocol();
				if (!continue_processing)
				{
					LogDebug(Channel::Protocol, std::format("Error occured during processing serial data in the protocol handler"));
				}

				pu_frame->End();

			} while (continue_processing);
		}

	public:
		boost::asio::awaitable<bool> HandleProtocol()
		{
			std::array<Interfaces::ISerialPort::DataType, 16> read_buffer;
			bool continue_processing = true;

			do
			{ 
				static_cast<void>(Factory::ProfilingUnitFactory::Instance().CreateZone("HandleProtocol -> Getting Serial Data", std::source_location::current(), Profiling::UnitColours::Red));

				auto buffer = boost::asio::buffer(read_buffer, read_buffer.size());
				auto [ec, bytes_read] = co_await m_SerialPort.async_read_some(buffer, boost::asio::as_tuple(boost::asio::use_awaitable));
				switch (ec.value())
				{
				case boost::system::errc::success:
					{
						static_cast<void>(Factory::ProfilingUnitFactory::Instance().CreateZone("HandleProtocol -> Injecting Serial Data", std::source_location::current(), Profiling::UnitColours::Green));
					
						auto read_buffer_span = std::span(read_buffer.begin(), bytes_read);
						LogTrace(Channel::Protocol, std::format("The following bytes were read from the serial device: {:.{}})", read_buffer_span, read_buffer_span.size()));
						LogTrace(Channel::Protocol, std::format("Successfully yield'd -> async_read_some() ({} bytes read)", bytes_read));

						m_Generator.InjectRawSerialData(read_buffer_span);
						bool process_packets = true;

						do
						{
							static_cast<void>(Factory::ProfilingUnitFactory::Instance().CreateZone("HandleProtocol -> Generating Messages", std::source_location::current(), Profiling::UnitColours::Blue));

							auto message = co_await m_Generator.GenerateMessageFromRawData();
							if (message.has_value())
							{
								LogTrace(Channel::Protocol, "Message received; emitting signal to listening consumer slots.");

								if (auto signal_ptr = std::dynamic_pointer_cast<Interfaces::IMessageSignalBase>(message.value()); nullptr != signal_ptr)
								{
									static_cast<void>(Factory::ProfilingUnitFactory::Instance().CreateZone("HandleProtocol -> Signalling Signals", std::source_location::current(), Profiling::UnitColours::Yellow));

									// Process the message...as per protocol requirements
									signal_ptr->Signal();
								}
							}
							else if (message.error() == make_error_code(ErrorCodes::Protocol_ErrorCodes::DataAvailableToProcess))
							{
								// Continue processing data looking for messages in the buffer...
							}
							else if (message.error() == make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData))
							{
								process_packets = false;
							}
							else if (message.error() == make_error_code(ErrorCodes::Message_ErrorCodes::Error_CannotFindGenerator))
							{
								// This means there was a packet that could not be deserialised while processing....
							}
							else
							{
								LogDebug(Logging::Channel::Protocol, "Protocol error while processing Jandy messages.");
								continue_processing = false;
								process_packets = false;
							}

						} while (process_packets);
					}
					break;

				case boost::asio::error::eof:
					LogDebug(Channel::Protocol, "Serial port's connection was closed by the peer...cannot continue.");
					continue_processing = false;
					break;

				case boost::system::errc::operation_canceled:
				case boost::asio::error::operation_aborted:
					LogDebug(Channel::Protocol, "Serial port's async_read_some() was cancelled or an error occurred.");
					continue_processing = false;
					break;

				default:
					LogError(Channel::Protocol, std::format("Error {} occured in protocol handler.  Error Code: {}", ec.to_string(), ec.value()));
					LogDebug(Channel::Protocol, std::format("Error message: {}", ec.message()));
					continue_processing = false;
					break;
				}
			
			} while (continue_processing);

			co_return false;
		}

	private:
		Profiling::DomainPtr m_ProfilingDomain;
		Serial::SerialPort& m_SerialPort;
		GENERATOR_TYPE& m_Generator;
	};
}
// namespace AqualinkAutomate::Protocol
