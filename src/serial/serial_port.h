#pragma once

#include <cstdint>
#include <string>

#include <boost/circular_buffer.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/write.hpp>

namespace AqualinkAutomate::Serial
{

	class SerialPort
	{
	public:
		SerialPort(boost::asio::io_context& io_context, std::string port) : 
			m_IOContext(io_context), 
			m_SerialPort(m_IOContext, port)
		{
		}

		void async_write(const std::string& data, boost::asio::yield_context yield) 
		{
			boost::asio::async_write(m_SerialPort, boost::asio::buffer(data), yield);
		}

		void async_read(boost::circular_buffer<uint8_t>& buffer, boost::asio::yield_context yield) 
		{
			// boost::asio::async_read(m_SerialPort, boost::asio::buffer(buffer), yield);
		}

	private:
		boost::asio::io_context& m_IOContext;
		boost::asio::serial_port m_SerialPort;
	};

}
// namespace AqualinkAutomate::Serial
