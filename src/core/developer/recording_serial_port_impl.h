#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>

#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>

#include "interfaces/iserialportimpl.h"
#include "logging/logging.h"
#include "serial/serial_port_enums.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Developer
{

	/// @brief Decorator that wraps any ISerialPortImpl and records serial data to a file.
	///
	/// This class intercepts read operations and writes the data to a file in a format
	/// compatible with MockSerialPortImpl for replay. The format is: 0x##|0x##|...
	///
	/// Recording includes timestamps to help with debugging timing-related issues.
	class RecordingSerialPortImpl : public Interfaces::ISerialPortImpl
	{
	public:
		/// @brief Constructs a recording wrapper around an existing serial port implementation.
		/// @param wrapped_impl The underlying serial port implementation to wrap.
		/// @param recording_file_path Path to the file where serial data will be recorded.
		RecordingSerialPortImpl(std::unique_ptr<Interfaces::ISerialPortImpl> wrapped_impl, const std::string& recording_file_path);

		~RecordingSerialPortImpl() override;

		RecordingSerialPortImpl(const RecordingSerialPortImpl&) = delete;
		RecordingSerialPortImpl& operator=(const RecordingSerialPortImpl&) = delete;
		RecordingSerialPortImpl(RecordingSerialPortImpl&& other) noexcept = delete;
		RecordingSerialPortImpl& operator=(RecordingSerialPortImpl&& other) noexcept = delete;

		void open(const std::string& device_name) override;
		void open(const std::string& device_name, boost::system::error_code& ec) override;
		bool is_open() const override;
		void cancel() override;
		void cancel(boost::system::error_code& ec) override;
		void close() override;
		void close(boost::system::error_code& ec) override;

		void set_baud_rate(uint32_t rate, boost::system::error_code& ec) override;
		void set_character_size(uint8_t bits, boost::system::error_code& ec) override;
		void set_flow_control(Serial::FlowControl fc, boost::system::error_code& ec) override;
		void set_parity(Serial::Parity p, boost::system::error_code& ec) override;
		void set_stop_bits(Serial::StopBits sb, boost::system::error_code& ec) override;
		void set_read_timeout(std::chrono::milliseconds timeout, boost::system::error_code& ec) override;

		std::size_t read_some(const boost::asio::mutable_buffer& buffer) override;
		std::size_t read_some(const boost::asio::mutable_buffer& buffer, boost::system::error_code& ec) override;
		std::size_t write_some(const boost::asio::const_buffer& buffer) override;
		std::size_t write_some(const boost::asio::const_buffer& buffer, boost::system::error_code& ec) override;

	private:
		void RecordReadData(const uint8_t* data, std::size_t length);
		void RecordWriteData(const uint8_t* data, std::size_t length);
		void WriteTimestamp();

	private:
		std::unique_ptr<Interfaces::ISerialPortImpl> m_WrappedImpl;
		std::string m_RecordingFilePath;
		std::ofstream m_RecordingFile;
		std::mutex m_FileMutex;
		std::chrono::steady_clock::time_point m_StartTime;
		bool m_RecordingEnabled{ false };
	};

}
// namespace AqualinkAutomate::Developer
