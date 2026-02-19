#include <boost/asio.hpp>
#include <boost/asio/basic_socket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>

#include "logging/logging.h"
#include "serial/port_types/network_serial_port_impl.h"
#include "serial/rfc2217/rfc2217_protocol_handler.h"
#include "utility/endpoint_parser.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Serial::PortTypes
{

	NetworkSerialPortImpl::NetworkSerialPortImpl(boost::asio::any_io_executor executor) :
		m_Executor(std::move(executor)),
		m_Socket(m_Executor),
		m_ProtocolHandler(CreateProtocolHandler()),
		m_ProfilingDomain(std::move(Factory::ProfilerFactory::Instance().Get()->CreateDomain("NetworkSerialPortImpl")))
	{
		LogTrace(Channel::Serial, "NetworkSerialPortImpl created");
	}

	NetworkSerialPortImpl::NetworkSerialPortImpl(boost::asio::any_io_executor executor, const std::string& endpoint_name) :
		NetworkSerialPortImpl(std::move(executor))
	{
		LogDebug(Channel::Serial, std::format("NetworkSerialPortImpl created with endpoint: {}", endpoint_name));
		open(endpoint_name);
	}

	NetworkSerialPortImpl::NetworkSerialPortImpl(boost::asio::any_io_executor executor, const std::string& endpoint_name, boost::system::error_code& ec) :
		NetworkSerialPortImpl(std::move(executor))
	{
		LogDebug(Channel::Serial, std::format("NetworkSerialPortImpl created with endpoint: {}", endpoint_name));
		open(endpoint_name, ec);
	}

	NetworkSerialPortImpl::~NetworkSerialPortImpl()
	{
		LogTrace(Channel::Serial, "NetworkSerialPortImpl destructor called");
		close();
	}

	std::unique_ptr<Interfaces::ISerialPortProtocol> NetworkSerialPortImpl::CreateProtocolHandler()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::CreateProtocolHandler", std::source_location::current());

		LogTrace(Channel::Serial, "Creating RFC2217 protocol handler");
		return std::make_unique<Serial::RFC2217::ProtocolHandler>(m_Socket);
	}

	void NetworkSerialPortImpl::open(const std::string& endpoint_name)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::open", std::source_location::current());

		boost::system::error_code ec;

		if (open(endpoint_name, ec); ec)
		{
			LogFatal(Channel::Serial, std::format("Failed to open network serial port '{}': {}", endpoint_name, ec.message()));
			throw boost::system::system_error(ec);
		}
	}

	void NetworkSerialPortImpl::open(const std::string& endpoint_name, boost::system::error_code& ec)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::open", std::source_location::current());

		LogInfo(Channel::Serial, std::format("Opening network serial port: {}", endpoint_name));

		ec = {};

		if (is_open())
		{
			LogWarning(Channel::Serial, std::format("Attempted to open already open network serial port: {}", endpoint_name));
			ec = boost::asio::error::already_open;
		}
		else if (auto [host, service] = Utility::ParseEndpoint(endpoint_name); host.empty() || service.empty())
		{
			LogWarning(Channel::Serial, std::format("Invalid endpoint format: {}", endpoint_name));
			ec = boost::asio::error::invalid_argument;
		}
		else
		{
			auto resolve_zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::open -> resolve", std::source_location::current());

			LogDebug(Channel::Serial, std::format("Resolving endpoint: {}:{}", host, service));
			boost::asio::ip::tcp::resolver resolver(m_Executor);

			if (auto results = resolver.resolve(host, service, ec); ec)
			{
				LogWarning(Channel::Serial, std::format("Failed to resolve endpoint '{}:{}': {}", host, service, ec.message()));
			}
			else
			{
				auto connect_zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::open -> connect", std::source_location::current());

				if (boost::asio::connect(m_Socket, results, ec); ec)
				{
					LogWarning(Channel::Serial, std::format("Failed to connect to '{}:{}': {}", host, service, ec.message()));
				}
				else
				{
					auto init_zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::open -> initialise_protocol", std::source_location::current());

					m_EndpointName = endpoint_name;
					m_IsOpen = true;

					// Set socket to non-blocking so read_some returns would_block
					// instead of blocking the frame loop when no data is available
					m_Socket.non_blocking(true, ec);
					if (ec)
					{
						LogWarning(Channel::Serial, std::format("Failed to set non-blocking mode on '{}': {}", endpoint_name, ec.message()));
						m_IsOpen = false;
						m_Socket.close();
						return;
					}

					// Disable Nagle's algorithm — serial-over-TCP messages are small
					// and latency-sensitive; buffering them defeats the purpose.
					boost::asio::ip::tcp::no_delay no_delay_opt(true);
					m_Socket.set_option(no_delay_opt, ec);
					if (ec)
					{
						LogWarning(Channel::Serial, std::format("Failed to set TCP_NODELAY on '{}': {}", endpoint_name, ec.message()));
						ec = {};
					}

					LogInfo(Channel::Serial, std::format("Successfully connected to network serial port: {}", endpoint_name));

					LogDebug(Channel::Serial, "Initializing RFC2217 protocol handler");
					m_ProtocolHandler->Initialize();
				}
			}
		}
	}

	bool NetworkSerialPortImpl::is_open() const
	{
		return m_IsOpen && m_Socket.is_open();
	}

	void NetworkSerialPortImpl::close()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::close", std::source_location::current());

		boost::system::error_code ec;

		if (close(ec); ec)
		{
			LogWarning(Channel::Serial, std::format("Error during network serial port close: {}", ec.message()));
			throw boost::system::system_error(ec);
		}
	}

	void NetworkSerialPortImpl::close(boost::system::error_code& ec)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::close", std::source_location::current());

		if (is_open())
		{
			LogInfo(Channel::Serial, std::format("Closing network serial port: {}", m_EndpointName));
		}
		else
		{
			LogTrace(Channel::Serial, "Close called on already closed network serial port");
		}

		ec = {};

		m_EndpointName.clear();
		m_IsOpen = false;

		if (m_ProtocolHandler)
		{
			auto shutdown_zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::close -> shutdown_protocol", std::source_location::current());
			LogTrace(Channel::Serial, "Shutting down protocol handler");
			m_ProtocolHandler->Shutdown();
		}

		boost::system::error_code ignored;
		if (m_Socket.is_open())
		{
			auto socket_zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::close -> shutdown_socket", std::source_location::current());
			LogTrace(Channel::Serial, "Shutting down TCP socket");
			m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored);
			m_Socket.close(ignored);
		}

		LogDebug(Channel::Serial, "Network serial port closed successfully");
	}

	void NetworkSerialPortImpl::cancel()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::cancel", std::source_location::current());

		boost::system::error_code ec;

		if (cancel(ec); ec)
		{
			LogWarning(Channel::Serial, std::format("Error during cancel operation: {}", ec.message()));
			throw boost::system::system_error(ec);
		}
	}

	void NetworkSerialPortImpl::cancel(boost::system::error_code& ec)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::cancel", std::source_location::current());

		if (!is_open())
		{
			LogWarning(Channel::Serial, "Attempted to cancel operations on closed network serial port");
			ec = boost::asio::error::bad_descriptor;
		}
		else
		{
			ec = {};

			LogDebug(Channel::Serial, "Cancelling pending operations");
			m_Socket.cancel(ec);

			if (ec)
			{
				LogWarning(Channel::Serial, std::format("Error cancelling operations: {}", ec.message()));
			}
		}
	}

	void NetworkSerialPortImpl::set_baud_rate(uint32_t rate, boost::system::error_code& ec)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::set_baud_rate", std::source_location::current());

		LogDebug(Channel::Serial, std::format("Setting baud rate: {}", rate));
		m_Options.baud_rate = rate;

		if (m_ProtocolHandler)
		{
			m_ProtocolHandler->set_baud_rate(rate);
		}
		else
		{
			LogWarning(Channel::Serial, "Attempted to set baud rate but protocol handler is null");
		}

		ec = {};
	}

	void NetworkSerialPortImpl::set_character_size(uint8_t bits, boost::system::error_code& ec)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::set_character_size", std::source_location::current());

		LogDebug(Channel::Serial, std::format("Setting character size: {}", bits));
		m_Options.character_size = bits;

		if (m_ProtocolHandler)
		{
			m_ProtocolHandler->set_character_size(bits);
		}
		else
		{
			LogWarning(Channel::Serial, "Attempted to set character size but protocol handler is null");
		}

		ec = {};
	}

	void NetworkSerialPortImpl::set_flow_control(Serial::FlowControl fc, boost::system::error_code& ec)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::set_flow_control", std::source_location::current());

		LogDebug(Channel::Serial, std::format("Setting flow control: {}", static_cast<int>(fc)));
		m_Options.flow_control = fc;

		if (m_ProtocolHandler)
		{
			m_ProtocolHandler->set_flow_control(fc);
		}
		else
		{
			LogWarning(Channel::Serial, "Attempted to set flow control but protocol handler is null");
		}

		ec = {};
	}

	void NetworkSerialPortImpl::set_parity(Serial::Parity p, boost::system::error_code& ec)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::set_parity", std::source_location::current());

		LogDebug(Channel::Serial, std::format("Setting parity: {}", static_cast<int>(p)));
		m_Options.parity = p;

		if (m_ProtocolHandler)
		{
			m_ProtocolHandler->set_parity(p);
		}
		else
		{
			LogWarning(Channel::Serial, "Attempted to set parity but protocol handler is null");
		}

		ec = {};
	}

	void NetworkSerialPortImpl::set_stop_bits(Serial::StopBits sb, boost::system::error_code& ec)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::set_stop_bits", std::source_location::current());

		LogDebug(Channel::Serial, std::format("Setting stop bits: {}", static_cast<int>(sb)));
		m_Options.stop_bits = sb;

		if (m_ProtocolHandler)
		{
			m_ProtocolHandler->set_stop_bits(sb);
		}
		else
		{
			LogWarning(Channel::Serial, "Attempted to set stop bits but protocol handler is null");
		}

		ec = {};
	}

	std::size_t NetworkSerialPortImpl::read_some(const boost::asio::mutable_buffer& b)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::read_some", std::source_location::current());

		boost::system::error_code ec;
		auto bytes = read_some(b, ec);
		if (ec)
		{
			LogWarning(Channel::Serial, std::format("Synchronous read_some failed: {}", ec.message()));
			throw boost::system::system_error(ec);
		}
		return bytes;
	}

	std::size_t NetworkSerialPortImpl::read_some(const boost::asio::mutable_buffer& b, boost::system::error_code& ec)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::read_some", std::source_location::current());

		if (!is_open())
		{
			LogWarning(Channel::Serial, "Attempted to read from closed network serial port");
			ec = boost::asio::error::bad_descriptor;
			return 0;
		}

		auto bytes_read = m_Socket.read_some(boost::asio::buffer(b), ec);
		if (ec)
		{
			if (ec == boost::asio::error::would_block || ec == boost::asio::error::try_again)
			{
				// Normal for non-blocking socket when no data is available
				return 0;
			}
			LogWarning(Channel::Serial, std::format("Socket read failed: {}", ec.message()));
			return 0;
		}

		LogTrace(Channel::Serial, std::format("Read {} bytes from network socket", bytes_read));
		return bytes_read;
	}

	std::size_t NetworkSerialPortImpl::write_some(const boost::asio::const_buffer& b)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::write_some", std::source_location::current());

		boost::system::error_code ec;
		auto bytes = write_some(b, ec);
		if (ec)
		{
			LogWarning(Channel::Serial, std::format("Synchronous write_some failed: {}", ec.message()));
			throw boost::system::system_error(ec);
		}
		return bytes;
	}

	std::size_t NetworkSerialPortImpl::write_some(const boost::asio::const_buffer& b, boost::system::error_code& ec)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("NetworkSerialPortImpl::write_some", std::source_location::current());

		if (!is_open())
		{
			LogWarning(Channel::Serial, "Attempted to write to closed network serial port");
			ec = boost::asio::error::bad_descriptor;
			return 0;
		}

		auto bytes_written = m_Socket.write_some(boost::asio::buffer(b), ec);
		if (ec)
		{
			if (ec == boost::asio::error::would_block || ec == boost::asio::error::try_again)
			{
				// Normal for non-blocking socket when send buffer is full
				return 0;
			}
			LogWarning(Channel::Serial, std::format("Socket write failed: {}", ec.message()));
			return 0;
		}

		LogTrace(Channel::Serial, std::format("Wrote {} bytes to network socket", bytes_written));
		return bytes_written;
	}

}
// namespace AqualinkAutomate::Serial::PortTypes
