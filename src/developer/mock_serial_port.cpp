#include "developer/mock_serial_port.h"

namespace AqualinkAutomate::Developer
{
	mock_serial_port::mock_serial_port(boost::asio::io_context& io_context) :
		m_IOContext(io_context),
		m_WriteDelayTimer(io_context),
		m_RandomDevice{},
		m_Distribution(32, 127),
		m_ProfilingDomain(std::move(Factory::ProfilerFactory::Instance().Get()->CreateDomain("mock_serial_port").value()))
	{
	}

	mock_serial_port::mock_serial_port(boost::asio::io_context& io_context, const std::string& device_name)
		: mock_serial_port(io_context)
	{
		open(device_name);
	}

	mock_serial_port::mock_serial_port(boost::asio::io_context& io_context, const std::string& device_name, boost::system::error_code& ec)
		: mock_serial_port(io_context)
	{
		open(device_name, ec);
	}

	mock_serial_port::~mock_serial_port()
	{
		close();
	}

	void mock_serial_port::open(const std::string& device_name)
	{
		// We're not actually opening a real serial port, so this function
		// doesn't do anything except set the "is_open" flag.

		boost::system::error_code ec;
		open(device_name, ec);
		if (ec)
		{
			throw boost::system::system_error(ec);
		}
	}

	void mock_serial_port::open(const std::string& device_name, boost::system::error_code& ec)
	{
		// We're not actually opening a real serial port, so this function
		// doesn't do anything except set the "is_open" flag.

		ec = {};

		if (is_open())
		{
			ec = boost::asio::error::already_open;
		}
		else if (m_File.open(device_name); m_File.is_open())
		{
			m_MockData = false;
			m_IsOpen = true;
			m_DeviceName = device_name;
		}
		else
		{
			m_MockData = true;
			m_IsOpen = true;
			m_DeviceName = device_name;
		}
	}

	bool mock_serial_port::is_open() const
	{
		return m_IsOpen;
	}

	void mock_serial_port::close()
	{
		// Again, we're not actually closing a real serial port, so this
		// function just sets the "is_open" flag to false.

		boost::system::error_code ec;
		close(ec);
		if (ec)
		{
			throw boost::system::system_error(ec);
		}
	}

	void mock_serial_port::close(boost::system::error_code& ec)
	{
		// Again, we're not actually closing a real serial port, so this
		// function just sets the "is_open" flag to false.

		ec = {};

		m_DeviceName.clear();
		m_IsOpen = false;
		m_MockData = true;

		if (m_File && m_File.is_open()) m_File.close();
	}

	void mock_serial_port::cancel()
	{
		boost::system::error_code ec;
		cancel(ec);
		if (ec)
		{
			throw boost::system::system_error(ec);
		}
	}

	void mock_serial_port::cancel(boost::system::error_code& ec)
	{
		if (!m_IsOpen)
		{
			ec = boost::asio::error::bad_descriptor;
		}
		else
		{
			m_WriteDelayTimer.cancel(ec);
		}
	}

	boost::asio::executor mock_serial_port::get_executor()
	{
		return m_IOContext.get_executor();
	}

	void mock_serial_port::set_option(const boost::asio::serial_port_base::baud_rate& option, boost::system::error_code& ec)
	{
		m_Options.baud_rate = option;
	}

	void mock_serial_port::set_option(const boost::asio::serial_port_base::character_size& option, boost::system::error_code& ec)
	{
		m_Options.character_size = option;
	}

	void mock_serial_port::set_option(const boost::asio::serial_port_base::flow_control& option, boost::system::error_code& ec)
	{
		m_Options.flow_control = option;
	}

	void mock_serial_port::set_option(const boost::asio::serial_port_base::parity& option, boost::system::error_code& ec)
	{
		m_Options.parity = option;
	}

	void mock_serial_port::set_option(const boost::asio::serial_port_base::stop_bits& option, boost::system::error_code& ec)
	{
		m_Options.stop_bits = option;
	}

}
// namespace AqualinkAutomate::Developer
