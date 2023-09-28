#include "serial/serial_port.h"

namespace AqualinkAutomate::Serial
{

	SerialPort::SerialPort(Types::ExecutorType executor, Kernel::HubLocator& hub_locator, OperatingModes operating_mode) :
		m_OperatingMode(operating_mode),
		m_MockSerialPort(std::make_unique<MockSerialPort>(executor)),
		m_RealSerialPort(std::make_unique<RealSerialPort>(executor)),
		m_StatisticsHub(hub_locator.Find<Kernel::StatisticsHub>())
	{
	}

	void SerialPort::open(const std::string& device)
	{
		boost::system::error_code ec;
		open(device, ec);
		boost::asio::detail::throw_error(ec, "SerialPort::open");
	}

	void SerialPort::open(const std::string& device, boost::system::error_code& ec)
	{
		switch (m_OperatingMode)
		{
		case OperatingModes::Mock: m_MockSerialPort->open(device, ec); break;
		case OperatingModes::Real: m_RealSerialPort->open(device, ec); break;
		default: throw Exceptions::Serial_InvalidMode();
		}
	}

	bool SerialPort::is_open() const
	{
		switch (m_OperatingMode)
		{
		case OperatingModes::Mock: return m_MockSerialPort->is_open();
		case OperatingModes::Real: return m_RealSerialPort->is_open();
		default: throw Exceptions::Serial_InvalidMode();
		}
	}

	void SerialPort::cancel()
	{
		boost::system::error_code ec;
		cancel(ec);
		boost::asio::detail::throw_error(ec, "SerialPort::cancel");
	}

	void SerialPort::cancel(boost::system::error_code& ec)
	{
		switch (m_OperatingMode)
		{
		case OperatingModes::Mock: m_MockSerialPort->cancel(ec); break;
		case OperatingModes::Real: m_RealSerialPort->cancel(ec); break;
		default: throw Exceptions::Serial_InvalidMode();
		}
	}

	void SerialPort::close()
	{
		boost::system::error_code ec;
		close(ec);
		boost::asio::detail::throw_error(ec, "SerialPort::close");
	}
	
	void SerialPort::close(boost::system::error_code& ec)
	{
		switch (m_OperatingMode)
		{
		case OperatingModes::Mock: m_MockSerialPort->close(ec); break;
		case OperatingModes::Real: m_RealSerialPort->close(ec); break;
		default: throw Exceptions::Serial_InvalidMode();
		}
	}

}
// namespace AqualinkAutomate::Serial
