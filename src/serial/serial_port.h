#pragma once

#include <coroutine>
#include <optional>
#include <memory>
#include <span>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/system/error_code.hpp>

#include "developer/mock_serial_port.h"
#include "serial/serial_operating_modes.h"

namespace AqualinkAutomate::Serial
{

	class SerialPort : public std::enable_shared_from_this<SerialPort>
	{
		using MockSerialPort = AqualinkAutomate::Developer::mock_serial_port;
		using MockSerialPortPtr = std::shared_ptr<MockSerialPort>;
		using RealSerialPort = boost::asio::serial_port;
		using RealSerialPortPtr = std::shared_ptr<RealSerialPort>;

	public:
		SerialPort(boost::asio::io_context& io_context, OperatingModes operating_mode = OperatingModes::Real) :
			m_IOContext(io_context),
			m_OperatingMode(operating_mode),
			m_MockSerialPort(std::make_shared<MockSerialPort>(io_context)),
			m_RealSerialPort(std::make_shared<RealSerialPort>(io_context))
		{
		}

	public:
		void open(const std::string& device)
		{
			boost::system::error_code ec;
			open(device, ec);
			boost::asio::detail::throw_error(ec, "SerialPort::open");
		}

		void open(const std::string& device, boost::system::error_code& ec)
		{
			switch (m_OperatingMode)
			{
			case OperatingModes::Mock: m_MockSerialPort->open(device, ec); break;
			case OperatingModes::Real: m_RealSerialPort->open(device, ec); break;
			}
		}

		bool is_open() const
		{
			switch (m_OperatingMode)
			{
			case OperatingModes::Mock: return m_MockSerialPort->is_open();
			case OperatingModes::Real: return m_RealSerialPort->is_open();
			default:
				///FIXME
				throw;
			}
		}

		void cancel()
		{
			boost::system::error_code ec;
			cancel(ec);
			boost::asio::detail::throw_error(ec, "SerialPort::cancel");
		}

		void cancel(boost::system::error_code& ec)
		{
			switch (m_OperatingMode)
			{
			case OperatingModes::Mock: m_MockSerialPort->cancel(ec); break;
			case OperatingModes::Real: m_RealSerialPort->cancel(ec); break;
			}
		}

		void close()
		{
			boost::system::error_code ec;
			close(ec);
			boost::asio::detail::throw_error(ec, "SerialPort::close");
		}

		void close(boost::system::error_code& ec)
		{
			switch (m_OperatingMode)
			{
			case OperatingModes::Mock: m_MockSerialPort->close(ec); break;
			case OperatingModes::Real: m_RealSerialPort->close(ec); break;
			}
		}

		template<typename MutableBufferSequence, boost::asio::completion_token_for<void(boost::system::error_code, std::size_t)> ReadToken>
		auto async_read_some(const MutableBufferSequence& buffer, ReadToken&& token)
		{
			switch (m_OperatingMode)
			{
			case OperatingModes::Mock:
				return m_MockSerialPort->async_read_some(buffer, std::forward<ReadToken>(token));
				break;

			case OperatingModes::Real:
				return m_RealSerialPort->async_read_some(buffer, std::forward<ReadToken>(token));
				break;

			default:
				///FIXME
				throw;
			}
		}

		template<typename ConstBufferSequence, boost::asio::completion_token_for<void(boost::system::error_code, std::size_t)> WriteToken>
		auto async_write_some(const ConstBufferSequence& buffer, WriteToken&& token)
		{
			switch (m_OperatingMode)
			{
			case OperatingModes::Mock:
				return m_MockSerialPort->async_write_some(buffer, std::forward<WriteToken>(token));
				break;

			case OperatingModes::Real:
				return m_RealSerialPort->async_write_some(buffer, std::forward<WriteToken>(token));
				break;

			default:
				///FIXME
				throw;
			}
		}

		template<typename SettableSerialPortOption>
		void set_option(const SettableSerialPortOption& option, boost::system::error_code& ec)
		{
			switch (m_OperatingMode)
			{
			case OperatingModes::Mock: m_MockSerialPort->set_option(option, ec); break;
			case OperatingModes::Real: m_RealSerialPort->set_option(option, ec); break;
			}
		}

	private:
		const OperatingModes m_OperatingMode;

	private:
		boost::asio::io_context& m_IOContext;
		MockSerialPortPtr m_MockSerialPort;
		RealSerialPortPtr m_RealSerialPort;
	};

}
// namespace AqualinkAutomate::Serial
