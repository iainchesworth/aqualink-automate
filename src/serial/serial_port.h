#pragma once

#include <coroutine>
#include <cstdint>
#include <optional>
#include <memory>
#include <span>
#include <string>

#include <boost/asio/serial_port.hpp>
#include <boost/system/error_code.hpp>

#include "developer/mock_serial_port.h"
#include "exceptions/exception_serial_invalidmode.h"
#include "interfaces/iserialport.h"
#include "kernel/hub_locator.h"
#include "kernel/statistics_hub.h"
#include "profiling/profiling.h"
#include "serial/serial_operating_modes.h"

namespace AqualinkAutomate::Serial
{

	class SerialPort : public Interfaces::ISerialPort, public std::enable_shared_from_this<SerialPort>
	{
		using MockSerialPort = AqualinkAutomate::Developer::mock_serial_port;
		using MockSerialPortPtr = std::unique_ptr<MockSerialPort>;
		using RealSerialPort = boost::asio::serial_port;
		using RealSerialPortPtr = std::unique_ptr<RealSerialPort>;

	public:
		SerialPort(Types::ExecutorType executor, Kernel::HubLocator& hub_locator, OperatingModes operating_mode = OperatingModes::Real);

	public:
		void open(const std::string& device);
		void open(const std::string& device, boost::system::error_code& ec);
		bool is_open() const;
		void cancel();
		void cancel(boost::system::error_code& ec);
		void close();
		void close(boost::system::error_code& ec);

		template<typename MutableBufferSequence, boost::asio::completion_token_for<void(boost::system::error_code, std::size_t)> ReadToken>
		auto async_read_some(const MutableBufferSequence& buffer, ReadToken&& token)
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("SerialPort -> async_read_some", BOOST_CURRENT_LOCATION);

			switch (m_OperatingMode)
			{
			case OperatingModes::Mock:
			{
				auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("SerialPort -> mock serial port -> async_read_some", BOOST_CURRENT_LOCATION);
				return m_MockSerialPort->async_read_some(buffer, std::forward<ReadToken>(token));
			}

			case OperatingModes::Real:
			{
				auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("SerialPort -> boost serial port -> async_read_some", BOOST_CURRENT_LOCATION);
				return m_RealSerialPort->async_read_some(buffer, std::forward<ReadToken>(token));
			}

			default:
				throw Exceptions::Serial_InvalidMode();
			}
		}

		template<typename MutableBufferSequence>
		std::size_t read_some(const MutableBufferSequence& buffers, boost::system::error_code& ec)
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("SerialPort -> read_some", BOOST_CURRENT_LOCATION);
			std::size_t bytes_read{ 0 };

			switch (m_OperatingMode)
			{
			case OperatingModes::Mock:
			{
				auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("SerialPort -> mock serial port -> read_some", BOOST_CURRENT_LOCATION);
				throw;
			}

			case OperatingModes::Real:
			{
				auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("SerialPort -> boost serial port -> read_some", BOOST_CURRENT_LOCATION);
				bytes_read = m_RealSerialPort->read_some(buffers, ec);
				break;
			}

			default:
				throw Exceptions::Serial_InvalidMode();
			}

			m_StatisticsHub->BandwidthMetrics.Read += bytes_read;
			return bytes_read;
		}

		template<typename ConstBufferSequence, boost::asio::completion_token_for<void(boost::system::error_code, std::size_t)> WriteToken>
		auto async_write_some(const ConstBufferSequence& buffer, WriteToken&& token)
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("SerialPort -> async_write_some", BOOST_CURRENT_LOCATION);

			switch (m_OperatingMode)
			{
			case OperatingModes::Mock:
			{
				auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("SerialPort -> mock serial port -> async_write_some", BOOST_CURRENT_LOCATION);
				return m_MockSerialPort->async_write_some(buffer, std::forward<WriteToken>(token));
			}

			case OperatingModes::Real:
			{
				auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("SerialPort -> boost serial port -> async_write_some", BOOST_CURRENT_LOCATION);
				return m_RealSerialPort->async_write_some(buffer, std::forward<WriteToken>(token));
			}

			default:
				throw Exceptions::Serial_InvalidMode();
			}
		}

		template<typename ConstBufferSequence> 
		std::size_t write_some(const ConstBufferSequence& buffers, boost::system::error_code& ec)
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("SerialPort -> write_some", BOOST_CURRENT_LOCATION);
			std::size_t bytes_written{ 0 };

			switch (m_OperatingMode)
			{
			case OperatingModes::Mock:
			{
				auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("SerialPort -> mock serial port -> write_some", BOOST_CURRENT_LOCATION);
				throw;
			}

			case OperatingModes::Real:
			{
				auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("SerialPort -> boost serial port -> write_some", BOOST_CURRENT_LOCATION);
				bytes_written = m_RealSerialPort->write_some(buffers, ec);
				break;
			}

			default:
				throw Exceptions::Serial_InvalidMode();
			}

			m_StatisticsHub->BandwidthMetrics.Write += bytes_written;
			return bytes_written;
		}

		template<typename SettableSerialPortOption>
		void set_option(const SettableSerialPortOption& option, boost::system::error_code& ec)
		{
			switch (m_OperatingMode)
			{
			case OperatingModes::Mock: m_MockSerialPort->set_option(option, ec); break;
			case OperatingModes::Real: m_RealSerialPort->set_option(option, ec); break;
			default: throw Exceptions::Serial_InvalidMode();
			}
		}

	private:
		const OperatingModes m_OperatingMode;

	private:
		MockSerialPortPtr m_MockSerialPort;
		RealSerialPortPtr m_RealSerialPort;
	
	private:
		std::shared_ptr<Kernel::StatisticsHub> m_StatisticsHub;
	};

}
// namespace AqualinkAutomate::Serial
