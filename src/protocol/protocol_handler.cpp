#include <array>
#include <cstdint>
#include <format>
#include <ranges>

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/buffer.hpp>

#include "logging/logging.h"
#include "protocol/protocol_handler.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Serial;

template<>
struct std::formatter<std::array<uint8_t, 1024>> : std::formatter<std::string>
{

	template<typename Context>
	auto format(const std::array<uint8_t, 1024>& arr, Context& context) const
	{
		std::for_each_n(arr.begin(), 16, [&context](auto& elem)
			{
				context.out() = elem;
			}
		);

		return context.out();
	}
};

namespace AqualinkAutomate::Protocol
{

	ProtocolHandler::ProtocolHandler(std::shared_ptr<AqualinkAutomate::Serial::SerialPort> serial_port) :
		m_SerialPort(serial_port),
		m_MessageDataBuffer(READ_BUFFER_LEN),
		m_MessageStateMachine(),
		m_MessageDataBufferMutex()
	{
		m_MessageStateMachine.initiate();
	}

	boost::asio::awaitable<void> ProtocolHandler::HandleProtocol()
	{
		std::array<uint8_t, READ_BUFFER_LEN> read_buffer;
		bool continue_processing = true;

		do
		{
			auto buffer = boost::asio::buffer(read_buffer, read_buffer.size());
			auto [ec, bytes_read] = co_await m_SerialPort->async_read_some(buffer, boost::asio::as_tuple(boost::asio::use_awaitable));
			switch (ec.value())
			{
			case boost::system::errc::success:
				LogDebug(Channel::Serial, std::format("Successfully yield'd -> async_read_some() ({} bytes read)", bytes_read));
				LogTrace(Channel::Serial, std::format("The following bytes were read from the serial device: {:.{}})", read_buffer, bytes_read));
				{
					const std::lock_guard<std::mutex> lock(m_MessageDataBufferMutex);
					//std::copy(read_buffer.cbegin(), read_buffer.cbegin() + bytes_read, m_MessageDataBuffer.front());

					// Convert to a message

					// Do protocol goodness.
				}
				break; 
			
			case boost::system::errc::operation_canceled:
			case boost::asio::error::operation_aborted:
				LogDebug(Channel::Serial, "Serial port's async_read_some() was cancelled.");
				continue_processing = false;
				break;

			default:
				LogError(Channel::Serial, std::format("Error {} occured in protocol handler.  Error Code: {}", ec.to_string(), ec.value()));
				LogDebug(Channel::Serial, std::format("Error message: {}", ec.message()));
				continue_processing = false;
				break;
			}

		} while (continue_processing);
	}

}
// namespace AqualinkAutomate::Protocol

