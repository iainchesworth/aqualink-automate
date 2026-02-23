#include <chrono>

#include <boost/asio/error.hpp>

#include "serial/serial_port.h"

namespace AqualinkAutomate::Serial
{

	SerialPort::SerialPort(std::unique_ptr<Interfaces::ISerialPortImpl> serial_port_impl, Kernel::HubLocator& hub_locator) :
		m_SerialPortImpl(std::move(serial_port_impl)),
		m_StatisticsHub(hub_locator.Find<Kernel::StatisticsHub>())
	{
	}

	void SerialPort::open(const std::string& device)
	{
		boost::system::error_code ec;
		open(device, ec);
		if (ec) throw boost::system::system_error(ec, "SerialPort::open");
	}

	void SerialPort::open(const std::string& device, boost::system::error_code& ec)
	{
		m_SerialPortImpl->open(device, ec);
	}

	bool SerialPort::is_open() const
	{
		return m_SerialPortImpl->is_open();
	}

	void SerialPort::cancel()
	{
		boost::system::error_code ec;
		cancel(ec);
		if (ec) throw boost::system::system_error(ec, "SerialPort::cancel");
	}

	void SerialPort::cancel(boost::system::error_code& ec)
	{
		m_SerialPortImpl->cancel(ec);
	}

	void SerialPort::close()
	{
		boost::system::error_code ec;
		close(ec);
		if (ec) throw boost::system::system_error(ec, "SerialPort::close");
	}

	void SerialPort::close(boost::system::error_code& ec)
	{
		m_SerialPortImpl->close(ec);
	}

	void SerialPort::set_baud_rate(uint32_t rate)
	{
		boost::system::error_code ec;
		set_baud_rate(rate, ec);
		if (ec) throw boost::system::system_error(ec);
	}

	void SerialPort::set_baud_rate(uint32_t rate, boost::system::error_code& ec)
	{
		m_SerialPortImpl->set_baud_rate(rate, ec);
	}

	void SerialPort::set_character_size(uint8_t bits)
	{
		boost::system::error_code ec;
		set_character_size(bits, ec);
		if (ec) throw boost::system::system_error(ec);
	}

	void SerialPort::set_character_size(uint8_t bits, boost::system::error_code& ec)
	{
		m_SerialPortImpl->set_character_size(bits, ec);
	}

	void SerialPort::set_flow_control(FlowControl fc)
	{
		boost::system::error_code ec;
		set_flow_control(fc, ec);
		if (ec) throw boost::system::system_error(ec);
	}

	void SerialPort::set_flow_control(FlowControl fc, boost::system::error_code& ec)
	{
		m_SerialPortImpl->set_flow_control(fc, ec);
	}

	void SerialPort::set_parity(Parity p)
	{
		boost::system::error_code ec;
		set_parity(p, ec);
		if (ec) throw boost::system::system_error(ec);
	}

	void SerialPort::set_parity(Parity p, boost::system::error_code& ec)
	{
		m_SerialPortImpl->set_parity(p, ec);
	}

	void SerialPort::set_stop_bits(StopBits sb)
	{
		boost::system::error_code ec;
		set_stop_bits(sb, ec);
		if (ec) throw boost::system::system_error(ec);
	}

	void SerialPort::set_stop_bits(StopBits sb, boost::system::error_code& ec)
	{
		m_SerialPortImpl->set_stop_bits(sb, ec);
	}

	void SerialPort::set_read_timeout(std::chrono::milliseconds timeout)
	{
		boost::system::error_code ec;
		set_read_timeout(timeout, ec);
		if (ec) throw boost::system::system_error(ec);
	}

	void SerialPort::set_read_timeout(std::chrono::milliseconds timeout, boost::system::error_code& ec)
	{
		m_SerialPortImpl->set_read_timeout(timeout, ec);
	}

	std::size_t SerialPort::read_some(const boost::asio::mutable_buffer& buffers)
	{
		boost::system::error_code ec;
		const auto n = read_some(buffers, ec);
		if (ec) throw boost::system::system_error(ec, "SerialPort::read_some");
		return n;
	}

	std::size_t SerialPort::read_some(const boost::asio::mutable_buffer& buffers, boost::system::error_code& ec)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("SerialPort::read_some", std::source_location::current());

		auto start_time = std::chrono::steady_clock::now();
		std::size_t bytes_read = m_SerialPortImpl->read_some(buffers, ec);
		if (!ec)
		{
			zone->Value(bytes_read);
			m_StatisticsHub->BandwidthMetrics.Read += bytes_read;
			m_StatisticsHub->LatencyMetrics.ReadLatency.RecordSince(start_time);
		}

		return bytes_read;
	}

	std::size_t SerialPort::write_some(const boost::asio::const_buffer& buffers)
	{
		boost::system::error_code ec;
		const auto n = write_some(buffers, ec);
		if (ec) throw boost::system::system_error(ec, "SerialPort::write_some");
		return n;
	}

	std::size_t SerialPort::write_some(const boost::asio::const_buffer& buffers, boost::system::error_code& ec)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("SerialPort::write_some", std::source_location::current());

		auto start_time = std::chrono::steady_clock::now();
		std::size_t bytes_written = m_SerialPortImpl->write_some(buffers, ec);
		if (!ec)
		{
			zone->Value(bytes_written);
			m_StatisticsHub->BandwidthMetrics.Write += bytes_written;
			m_StatisticsHub->LatencyMetrics.WriteLatency.RecordSince(start_time);
		}

		return bytes_written;
	}

}
// namespace AqualinkAutomate::Serial
