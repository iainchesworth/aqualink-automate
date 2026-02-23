#pragma once

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <expected>
#include <format>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/system/system_error.hpp>

#include "developer/serial_port_options.h"
#include "interfaces/iserialportimpl.h"
#include "logging/logging.h"
#include "profiling/profiling.h"
#include "serial/serial_port_enums.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Developer
{

	class MockSerialPortImpl : public Interfaces::ISerialPortImpl
	{
	public:
		MockSerialPortImpl();
		explicit MockSerialPortImpl(const std::string& device_name);
		MockSerialPortImpl(const std::string& device_name, boost::system::error_code& ec);
		~MockSerialPortImpl() override;

		MockSerialPortImpl(const MockSerialPortImpl&) = delete;
		MockSerialPortImpl& operator=(const MockSerialPortImpl&) = delete;
		MockSerialPortImpl(MockSerialPortImpl&& other) noexcept = delete;
		MockSerialPortImpl& operator=(MockSerialPortImpl&& other) noexcept = delete;

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
		std::expected<std::size_t, boost::system::error_code> HandleMockRead(const boost::asio::mutable_buffer& buffer);
		std::expected<std::size_t, boost::system::error_code> HandleMockWrite(const boost::asio::const_buffer& buffer);

	private:
		std::expected<std::size_t, boost::system::error_code> HandleFileRead(const boost::asio::mutable_buffer& buffer);
		std::expected<std::size_t, boost::system::error_code> HandleFileWrite(const boost::asio::const_buffer& buffer);

	private:
		Developer::SerialPortOptions m_Options;

	private:
		std::random_device m_RandomDevice;
		std::uniform_int_distribution<> m_Distribution;
		std::string m_DeviceName;
		bool m_MockData{ true };
		bool m_IsOpen{ false };
		bool m_Cancelled{ false };

	private:
		boost::iostreams::stream<boost::iostreams::file_source> m_File;

	private:
		Profiling::DomainPtr m_ProfilingDomain;
	};

}
// namespace AqualinkAutomate::Developer
