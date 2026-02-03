#include <algorithm>
#include <array>
#include <chrono>
#include <format>

#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/circular_buffer.hpp>

#include "errors/protocol_errors.h"
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

	void ProtocolTask::DrainWrites()
	{
		while (!m_WriteQueue.empty())
		{
			auto& buffer = m_WriteQueue.front();
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("ProtocolThread::write_some", std::source_location::current());

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
						break;  // Try again next iteration
					}
					LogWarning(Channel::Protocol, std::format("Serial write error: {}", ec.message()));
					if (m_StatisticsHub) { ++m_StatisticsHub->Serial.TransmissionFailures; }
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
				break;  // Partial write; retry remainder next iteration
			}
			else
			{
				break;  // No progress; retry next iteration
			}
		}
	}

	void ProtocolTask::DrainReads()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("ProtocolThread::read_some", std::source_location::current());

		std::size_t total_bytes_read = 0;

		// Loop until no more data available — drain the OS serial buffer completely.
		for (;;)
		{
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
				break;
			}

			if (0 == bytes_read)
			{
				break;
			}

			total_bytes_read += bytes_read;

			for (std::size_t i = 0; i < bytes_read; ++i)
			{
				if (m_SerialBuffer.full())
				{
					LogWarning(Channel::Protocol, "Serial circular buffer overflow — oldest byte discarded");
					if (m_StatisticsHub) { ++m_StatisticsHub->MessageErrors.BufferOverflows; }
				}
				m_SerialBuffer.push_back(m_ReadBuffer[i]);
			}
		}

		if (total_bytes_read > 0)
		{
			zone->Value(total_bytes_read);

			auto profiler = Factory::ProfilerFactory::Instance().Get();
			profiler->PlotValue("Serial Bytes Read", static_cast<int64_t>(total_bytes_read));
			profiler->PlotValue("Serial Buffer Fill", static_cast<int64_t>(m_SerialBuffer.size()));

			LogDebug(Channel::Protocol, std::format("Read {} bytes from serial port", total_bytes_read));
		}
	}

	void ProtocolTask::Poll()
	{
		// Tight loop: read → process → write, so responses are sent in
		// the same frame as the message that triggered them.

		// 1. Drain any writes queued from the previous frame first.
		DrainWrites();

		// 2. Read → Process → Write loop.
		DrainReads();

		std::size_t messages_parsed = 0;
		if (!m_SerialBuffer.empty())
		{
			messages_parsed = ProcessMessages(m_SerialBuffer);

			// Immediately drain any responses that were queued by message handlers.
			DrainWrites();
		}

		// 3. Sample per-frame metrics for profiler and statistics.
		auto profiler = Factory::ProfilerFactory::Instance().Get();
		profiler->PlotValue("Write Queue Depth", static_cast<int64_t>(m_WriteQueue.size()));
		profiler->PlotValue("Messages Parsed", static_cast<int64_t>(messages_parsed));

		if (m_StatisticsHub)
		{
			m_StatisticsHub->Serial.SerialWriteQueueDepth = m_WriteQueue.size();

			profiler->PlotValue("TX Failures", static_cast<int64_t>(m_StatisticsHub->Serial.TransmissionFailures));
			profiler->PlotValue("Checksum Failures", static_cast<int64_t>(m_StatisticsHub->MessageErrors.ChecksumFailures));
			profiler->PlotValue("Deserialization Failures", static_cast<int64_t>(m_StatisticsHub->MessageErrors.DeserializationFailures));
			profiler->PlotValue("Invalid Packets", static_cast<int64_t>(m_StatisticsHub->MessageErrors.InvalidPacketFormat));
			profiler->PlotValue("Generator Failures", static_cast<int64_t>(m_StatisticsHub->MessageErrors.GeneratorFailures));
			profiler->PlotValue("Overlapping Packets", static_cast<int64_t>(m_StatisticsHub->MessageErrors.OverlappingPackets));
			profiler->PlotValue("Buffer Overflows", static_cast<int64_t>(m_StatisticsHub->MessageErrors.BufferOverflows));
		}
	}

	void ProtocolTask::EnqueueWrite(std::vector<uint8_t> buffer)
	{
		m_WriteQueue.push_back(std::move(buffer));
	}

	std::size_t ProtocolTask::ProcessMessages(ReadOps_SerialBufferType& circular_buffer)
	{
		std::size_t messages_parsed = 0;

		// Keep trying to parse messages from the buffer until we need more data
		while (!circular_buffer.empty())
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("ProtocolThread::ProcessMessages", std::source_location::current());

			auto msg_processing_start = std::chrono::steady_clock::now();

			auto message = MessageGeneratorRegistry::Instance().GenerateMessage(circular_buffer);
			if (message)
			{
				++messages_parsed;

				// Successfully parsed a message — fire the signal
				ProtocolHandler_ReadOp_MessageHandler(*message, m_StatisticsHub);

				if (m_StatisticsHub)
				{
					m_StatisticsHub->LatencyMetrics.MessageProcessingLatency.RecordSince(msg_processing_start);
				}
			}
			else
			{
				auto error = message.error();
				ProtocolHandler_ReadOp_ErrorHandler(error, m_StatisticsHub);

				// These errors mean the generator cleaned up bad data but there
				// may be another packet behind it — keep processing.
				auto ev = error.value();
				if (ev == ErrorCodes::Protocol_ErrorCodes::DataAvailableToProcess
					|| ev == ErrorCodes::Protocol_ErrorCodes::ChecksumFailure
					|| ev == ErrorCodes::Protocol_ErrorCodes::OverlappingPackets)
				{
					continue;
				}

				break;
			}
		}

		return messages_parsed;
	}

}
// namespace AqualinkAutomate::Protocol
