#include "developer/recording_serial_port_impl.h"

#include <format>
#include <iomanip>

#include "platform/safe_ctime.h"

namespace AqualinkAutomate::Developer
{

	RecordingSerialPortImpl::RecordingSerialPortImpl(
		std::unique_ptr<Interfaces::ISerialPortImpl> wrapped_impl,
		const std::string& recording_file_path)
		: m_WrappedImpl(std::move(wrapped_impl))
		, m_RecordingFilePath(recording_file_path)
		, m_StartTime(std::chrono::steady_clock::now())
	{
		LogInfo(Channel::Serial, std::format("Serial recording enabled, output file: {}", recording_file_path));

		m_RecordingFile.open(recording_file_path, std::ios::out | std::ios::trunc);
		if (m_RecordingFile.is_open())
		{
			m_RecordingEnabled = true;

			// Write header with timestamp
			auto now = std::chrono::system_clock::now();
			auto time_t_now = std::chrono::system_clock::to_time_t(now);
			char time_buf[26]{};
			Platform::SafeCtime(&time_t_now, time_buf, sizeof(time_buf));
			m_RecordingFile << "# Serial recording started at: " << time_buf;
			m_RecordingFile << "# Format: [timestamp_ms] direction byte|byte|byte|...\n";
			m_RecordingFile << "# Direction: R=read (from device), W=write (to device)\n";
			m_RecordingFile << "#\n";
			m_RecordingFile.flush();

			LogDebug(Channel::Serial, "Serial recording file opened successfully");
		}
		else
		{
			LogError(Channel::Serial, std::format("Failed to open serial recording file: {}", recording_file_path));
		}
	}

	RecordingSerialPortImpl::~RecordingSerialPortImpl()
	{
		if (m_RecordingFile.is_open())
		{
			m_RecordingFile << "# Recording ended\n";
			m_RecordingFile.close();
			LogInfo(Channel::Serial, std::format("Serial recording file closed: {}", m_RecordingFilePath));
		}
	}

	void RecordingSerialPortImpl::open(const std::string& device_name)
	{
		m_WrappedImpl->open(device_name);

		if (m_RecordingEnabled)
		{
			std::lock_guard<std::mutex> lock(m_FileMutex);
			WriteTimestamp();
			m_RecordingFile << "# Opened device: " << device_name << "\n";
			m_RecordingFile.flush();
		}
	}

	void RecordingSerialPortImpl::open(const std::string& device_name, boost::system::error_code& ec)
	{
		m_WrappedImpl->open(device_name, ec);

		if (m_RecordingEnabled && !ec)
		{
			std::lock_guard<std::mutex> lock(m_FileMutex);
			WriteTimestamp();
			m_RecordingFile << "# Opened device: " << device_name << "\n";
			m_RecordingFile.flush();
		}
	}

	bool RecordingSerialPortImpl::is_open() const
	{
		return m_WrappedImpl->is_open();
	}

	void RecordingSerialPortImpl::cancel()
	{
		m_WrappedImpl->cancel();
	}

	void RecordingSerialPortImpl::cancel(boost::system::error_code& ec)
	{
		m_WrappedImpl->cancel(ec);
	}

	void RecordingSerialPortImpl::close()
	{
		m_WrappedImpl->close();

		if (m_RecordingEnabled)
		{
			std::lock_guard<std::mutex> lock(m_FileMutex);
			WriteTimestamp();
			m_RecordingFile << "# Device closed\n";
			m_RecordingFile.flush();
		}
	}

	void RecordingSerialPortImpl::close(boost::system::error_code& ec)
	{
		m_WrappedImpl->close(ec);

		if (m_RecordingEnabled && !ec)
		{
			std::lock_guard<std::mutex> lock(m_FileMutex);
			WriteTimestamp();
			m_RecordingFile << "# Device closed\n";
			m_RecordingFile.flush();
		}
	}

	void RecordingSerialPortImpl::set_baud_rate(uint32_t rate, boost::system::error_code& ec)
	{
		m_WrappedImpl->set_baud_rate(rate, ec);

		if (m_RecordingEnabled && !ec)
		{
			std::lock_guard<std::mutex> lock(m_FileMutex);
			WriteTimestamp();
			m_RecordingFile << "# Set baud rate: " << rate << "\n";
			m_RecordingFile.flush();
		}
	}

	void RecordingSerialPortImpl::set_character_size(uint8_t bits, boost::system::error_code& ec)
	{
		m_WrappedImpl->set_character_size(bits, ec);

		if (m_RecordingEnabled && !ec)
		{
			std::lock_guard<std::mutex> lock(m_FileMutex);
			WriteTimestamp();
			m_RecordingFile << "# Set character size: " << static_cast<int>(bits) << "\n";
			m_RecordingFile.flush();
		}
	}

	void RecordingSerialPortImpl::set_flow_control(Serial::FlowControl fc, boost::system::error_code& ec)
	{
		m_WrappedImpl->set_flow_control(fc, ec);
	}

	void RecordingSerialPortImpl::set_parity(Serial::Parity p, boost::system::error_code& ec)
	{
		m_WrappedImpl->set_parity(p, ec);
	}

	void RecordingSerialPortImpl::set_stop_bits(Serial::StopBits sb, boost::system::error_code& ec)
	{
		m_WrappedImpl->set_stop_bits(sb, ec);
	}

	void RecordingSerialPortImpl::set_read_timeout(std::chrono::milliseconds timeout, boost::system::error_code& ec)
	{
		m_WrappedImpl->set_read_timeout(timeout, ec);
	}

	std::size_t RecordingSerialPortImpl::read_some(const boost::asio::mutable_buffer& buffer)
	{
		auto bytes_read = m_WrappedImpl->read_some(buffer);

		if (m_RecordingEnabled && bytes_read > 0)
		{
			RecordReadData(static_cast<const uint8_t*>(buffer.data()), bytes_read);
		}

		return bytes_read;
	}

	std::size_t RecordingSerialPortImpl::read_some(const boost::asio::mutable_buffer& buffer, boost::system::error_code& ec)
	{
		auto bytes_read = m_WrappedImpl->read_some(buffer, ec);

		if (m_RecordingEnabled && bytes_read > 0 && !ec)
		{
			RecordReadData(static_cast<const uint8_t*>(buffer.data()), bytes_read);
		}

		return bytes_read;
	}

	std::size_t RecordingSerialPortImpl::write_some(const boost::asio::const_buffer& buffer)
	{
		if (m_RecordingEnabled && buffer.size() > 0)
		{
			RecordWriteData(static_cast<const uint8_t*>(buffer.data()), buffer.size());
		}

		return m_WrappedImpl->write_some(buffer);
	}

	std::size_t RecordingSerialPortImpl::write_some(const boost::asio::const_buffer& buffer, boost::system::error_code& ec)
	{
		if (m_RecordingEnabled && buffer.size() > 0)
		{
			RecordWriteData(static_cast<const uint8_t*>(buffer.data()), buffer.size());
		}

		return m_WrappedImpl->write_some(buffer, ec);
	}

	void RecordingSerialPortImpl::RecordReadData(const uint8_t* data, std::size_t length)
	{
		std::lock_guard<std::mutex> lock(m_FileMutex);

		WriteTimestamp();
		m_RecordingFile << "R ";

		for (std::size_t i = 0; i < length; ++i)
		{
			m_RecordingFile << "0x" << std::hex << std::setw(2) << std::setfill('0')
				<< static_cast<int>(data[i]) << std::dec;
			if (i < length - 1)
			{
				m_RecordingFile << "|";
			}
		}
		m_RecordingFile << "\n";
		m_RecordingFile.flush();
	}

	void RecordingSerialPortImpl::RecordWriteData(const uint8_t* data, std::size_t length)
	{
		std::lock_guard<std::mutex> lock(m_FileMutex);

		WriteTimestamp();
		m_RecordingFile << "W ";

		for (std::size_t i = 0; i < length; ++i)
		{
			m_RecordingFile << "0x" << std::hex << std::setw(2) << std::setfill('0')
				<< static_cast<int>(data[i]) << std::dec;
			if (i < length - 1)
			{
				m_RecordingFile << "|";
			}
		}
		m_RecordingFile << "\n";
		m_RecordingFile.flush();
	}

	void RecordingSerialPortImpl::WriteTimestamp()
	{
		auto now = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_StartTime);
		m_RecordingFile << "[" << std::setw(10) << elapsed.count() << "] ";
	}

}
// namespace AqualinkAutomate::Developer
