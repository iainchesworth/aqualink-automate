#include <format>

#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/system/error_code.hpp>

#include "jandy/generator/jandy_message_generator.h"
#include "logging/logging.h"
#include "profiling/profiling.h"
#include "protocol/protocol_handler.h"
#include "utility/array_standard_formatter.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Protocol
{

	bool ProtocolHandler::HandleWrite()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("HandleWrite -> Writing Serial Data", BOOST_CURRENT_LOCATION, Profiling::UnitColours::Green);

		std::lock_guard<std::mutex> lock(m_SerialData_OutgoingMutex);

		bool continue_processing = true;
		boost::system::error_code ec;
		std::size_t bytes_written;

		bytes_written = m_SerialPort.write_some(boost::asio::buffer(m_SerialData_Outgoing), ec);
		switch (ec.value())
		{
		case boost::system::errc::success:
		{
			LogDebug(Channel::Protocol, std::format("Successfully wrote data to the serial port; bytes transmitted: {}", bytes_written));
			LogTrace(Channel::Protocol, std::format("The following bytes were written to the serial device: {}", m_SerialData_Outgoing));
			
			if (bytes_written == m_SerialData_Outgoing.size())
			{
				continue_processing = HandleWrite_Success(m_SerialData_Outgoing, bytes_written);
			}
			else
			{
				continue_processing = HandleWrite_Partial(m_SerialData_Outgoing, bytes_written);
			}			
			break;
		}

		case boost::asio::error::eof:
			LogDebug(Channel::Protocol, "Serial port's connection was closed by the peer...cannot continue.");
			continue_processing = false;
			break;

		case boost::asio::error::operation_aborted:
			LogDebug(Channel::Protocol, "Serial port's async_write_some() was cancelled or an error occurred.");
			continue_processing = false;
			break;

		default:
			LogError(Channel::Protocol, std::format("Error {} occured in protocol handler.  Error Code: {}", ec.to_string(), ec.value()));
			LogDebug(Channel::Protocol, std::format("Error message: {}", ec.message()));
			continue_processing = false;
			break;
		}

		return continue_processing;
	}

	bool ProtocolHandler::HandleWrite_Success(auto& write_buffer, auto bytes_written)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("HandleWrite -> Write Success; Clearing Internal Buffers", BOOST_CURRENT_LOCATION);
		LogTrace(Channel::Protocol, "Completed all serial data writing; clearing internal write buffer");
		m_SerialData_Outgoing.clear();

		return true;
	}

	bool ProtocolHandler::HandleWrite_Partial(auto& write_buffer, auto bytes_written)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("HandleWrite -> Write Partial; Handling Additional Transmission", BOOST_CURRENT_LOCATION);
		LogTrace(Channel::Protocol, std::format("Only partially completed serial data writing; {} bytes of {} bytes were transmitted", bytes_written, m_SerialData_Outgoing.size()));

		return true;
	}

}
// namespace AqualinkAutomate::Protocol
