#pragma once

#include <chrono>
#include <memory>
#include <random>
#include <string>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>

#include "developer/serial_port_options.h"
#include "interfaces/iserialportimpl.h"
#include "interfaces/iserialportprotocol.h"
#include "logging/logging.h"
#include "profiling/profiling.h"
#include "serial/rfc2217/rfc2217_types.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Serial::PortTypes
{

	class NetworkSerialPortImpl : public Interfaces::ISerialPortImpl
	{
	public:
		explicit NetworkSerialPortImpl(boost::asio::any_io_executor executor);
		NetworkSerialPortImpl(boost::asio::any_io_executor executor, const std::string& endpoint_name);
		NetworkSerialPortImpl(boost::asio::any_io_executor executor, const std::string& endpoint_name, boost::system::error_code& ec);
		~NetworkSerialPortImpl();

		NetworkSerialPortImpl(const NetworkSerialPortImpl&) = delete;
		NetworkSerialPortImpl& operator=(const NetworkSerialPortImpl&) = delete;
		NetworkSerialPortImpl(NetworkSerialPortImpl&&) noexcept = delete;
		NetworkSerialPortImpl& operator=(NetworkSerialPortImpl&&) noexcept = delete;

		void open(const std::string& endpoint_name) override;
		void open(const std::string& endpoint_name, boost::system::error_code& ec) override;
		bool is_open() const override;
		void close() override;
		void close(boost::system::error_code& ec) override;

		void cancel() override;
		void cancel(boost::system::error_code& ec) override;

		void set_baud_rate(uint32_t rate, boost::system::error_code& ec) override;
		void set_character_size(uint8_t bits, boost::system::error_code& ec) override;
		void set_flow_control(Serial::FlowControl fc, boost::system::error_code& ec) override;
		void set_parity(Serial::Parity p, boost::system::error_code& ec) override;
		void set_stop_bits(Serial::StopBits sb, boost::system::error_code& ec) override;
		void set_read_timeout(std::chrono::milliseconds timeout, boost::system::error_code& ec) override;

		std::size_t read_some(const boost::asio::mutable_buffer& b) override;
		std::size_t read_some(const boost::asio::mutable_buffer& b, boost::system::error_code& ec) override;
		std::size_t write_some(const boost::asio::const_buffer& b) override;
		std::size_t write_some(const boost::asio::const_buffer& b, boost::system::error_code& ec) override;

	private:
		Developer::SerialPortOptions m_Options;

	private:
		boost::asio::any_io_executor m_Executor;
		boost::asio::ip::tcp::socket m_Socket;
		std::random_device m_RandomDevice;
		std::string m_EndpointName;
		bool m_IsOpen{ false };

	private:
		std::unique_ptr<Interfaces::ISerialPortProtocol> CreateProtocolHandler();
		std::unique_ptr<Interfaces::ISerialPortProtocol> m_ProtocolHandler;

	private:
		Profiling::DomainPtr m_ProfilingDomain;
	};

}
// namespace AqualinkAutomate::Serial::PortTypes
