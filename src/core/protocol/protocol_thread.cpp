#include <algorithm>
#include <array>
#include <chrono>
#include <format>

#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/circular_buffer.hpp>

#include "logging/logging.h"
#include "profiling/profiling.h"
#include "protocol/message_generator_registry.h"
#include "protocol/protocol_handler_constants.h"
#include "protocol/protocol_handler_read.h"
#include "protocol/protocol_thread.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Protocol
{

	ProtocolTask::ProtocolTask(std::shared_ptr<Serial::SerialPort> serial_port,
							   std::shared_ptr<Kernel::StatisticsHub> statistics_hub)
		: m_SerialPort(std::move(serial_port))
		, m_StatisticsHub(std::move(statistics_hub))
		, m_SerialBuffer(Constants::SERIAL_CIRCULAR_BUFFER_SIZE)
	{
	}

	ProtocolTask::~ProtocolTask()
	{
		m_WriteSignalConnection.release();
	}

	void ProtocolTask::Poll()
	{
		// 1. Drain pending writes (no mutex needed — single-threaded)
		while (!m_WriteQueue.empty())
		{
			auto& buffer = m_WriteQueue.front();
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("ProtocolTask -> write_some", std::source_location::current());

			const std::size_t original_size = buffer.size();
			std::size_t total_bytes_written = 0;

			while (!buffer.empty())
			{
				boost::system::error_code ec;
				auto bytes_written = m_SerialPort->write_some(
					boost::asio::buffer(buffer.data(), buffer.size()), ec);

				if (ec)
				{
					if (ec == boost::asio::error::would_block || ec == boost::asio::error::try_again)
					{
						break;  // Try again next poll
					}
					LogWarning(Channel::Protocol, std::format("Serial write error: {}", ec.message()));
					break;
				}

				total_bytes_written += bytes_written;

				if (bytes_written >= buffer.size())
				{
					buffer.clear();
				}
				else
				{
					buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(bytes_written));
				}
			}

			zone->Value(total_bytes_written);
			Factory::ProfilerFactory::Instance().Get()->PlotValue("Serial Bytes Written", static_cast<int64_t>(total_bytes_written));

			if (original_size == total_bytes_written)
			{
				LogTrace(Channel::Protocol, std::format("Successfully wrote all {} bytes to serial port", total_bytes_written));
				m_WriteQueue.pop_front();
			}
			else if (total_bytes_written > 0)
			{
				LogDebug(Channel::Protocol, std::format("Write incomplete: wrote {} of {} bytes", total_bytes_written, original_size));
				break;  // Partial write; retry remainder next poll
			}
			else
			{
				break;  // No progress; retry next poll
			}
		}

		// 2. Non-blocking read from serial port
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("ProtocolTask -> read_some", std::source_location::current());

			boost::system::error_code ec;
			auto bytes_read = m_SerialPort->read_some(
				boost::asio::buffer(m_ReadBuffer.data(), m_ReadBuffer.size()), ec);

			if (ec)
			{
				if (ec == boost::asio::error::would_block || ec == boost::asio::error::try_again)
				{
					// No data available — normal for non-blocking
				}
				else if (ec == boost::asio::error::operation_aborted)
				{
					LogTrace(Channel::Protocol, "Serial read cancelled (shutdown)");
				}
				else if (ec == boost::asio::error::eof)
				{
					LogWarning(Channel::Protocol, "Serial port connection closed by peer");
				}
				else
				{
					LogTrace(Channel::Protocol, std::format("Serial read returned: {}", ec.message()));
				}
			}
			else if (bytes_read > 0)
			{
				zone->Value(bytes_read);

				auto profiler = Factory::ProfilerFactory::Instance().Get();
				profiler->PlotValue("Serial Bytes Read", static_cast<int64_t>(bytes_read));
				profiler->PlotValue("Serial Buffer Fill", static_cast<int64_t>(m_SerialBuffer.size() + bytes_read));

				LogDebug(Channel::Protocol, std::format("Read {} bytes from serial port", bytes_read));

				for (std::size_t i = 0; i < bytes_read; ++i)
				{
					m_SerialBuffer.push_back(m_ReadBuffer[i]);
				}
			}
		}

		// 3. Process any complete messages from the buffer
		if (!m_SerialBuffer.empty())
		{
			ProcessMessages(m_SerialBuffer);
		}
	}

	void ProtocolTask::EnqueueWrite(std::vector<uint8_t> buffer)
	{
		m_WriteQueue.push_back(std::move(buffer));
	}

	void ProtocolTask::ProcessMessages(ReadOps_SerialBufferType& circular_buffer)
	{
		// Keep trying to parse messages from the buffer until we need more data
		while (!circular_buffer.empty())
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("ProtocolTask -> ProcessMessages", std::source_location::current());

			auto msg_processing_start = std::chrono::steady_clock::now();

			auto message = MessageGeneratorRegistry::Instance().GenerateMessage(circular_buffer);
			if (message)
			{
				// Successfully parsed a message — fire the signal
				ProtocolHandler_ReadOp_MessageHandler(*message, m_StatisticsHub);

				if (m_StatisticsHub)
				{
					m_StatisticsHub->LatencyMetrics.MessageProcessingLatency.RecordSince(msg_processing_start);
				}
			}
			else
			{
				// No message could be parsed — need more data or encountered an error
				ProtocolHandler_ReadOp_ErrorHandler(message.error());
				break;
			}
		}
	}

}
// namespace AqualinkAutomate::Protocol
