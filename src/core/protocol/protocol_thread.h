#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <vector>

#include <boost/circular_buffer.hpp>
#include <boost/signals2.hpp>

#include "kernel/statistics_hub.h"
#include "protocol/protocol_handler_constants.h"
#include "protocol/protocol_handler_types.h"
#include "protocol/protocol_message_types.h"

namespace AqualinkAutomate::Protocol
{

	class ProtocolTask
	{
	public:
		// replay_frame_period paces the read/parse loop for capture replay (see
		// --replay-filename): when non-zero, each Poll() reads at most one serial
		// chunk and then sleeps so successive cycles are spaced by this period,
		// delivering frames at roughly the bus's natural rate instead of slurping
		// the whole capture in one cycle (which both defeats pacing and overflows
		// the circular buffer).  Zero (the default) = unpaced: drain everything
		// available each cycle with no sleep — correct for real ports and tests.
		ProtocolTask(std::shared_ptr<Serial::SerialPort> serial_port,
					 std::shared_ptr<Kernel::StatisticsHub> statistics_hub,
					 std::chrono::microseconds replay_frame_period = std::chrono::microseconds::zero());
		~ProtocolTask();

		ProtocolTask(const ProtocolTask&) = delete;
		ProtocolTask& operator=(const ProtocolTask&) = delete;
		ProtocolTask(ProtocolTask&&) = delete;
		ProtocolTask& operator=(ProtocolTask&&) = delete;

		/// Advance the protocol task.  Returns true if work was performed
		/// (messages parsed or writes pending), signalling the caller that
		/// sleeping can be skipped for tighter response latency.
		[[nodiscard]] bool Poll();

		void EnqueueWrite(std::vector<uint8_t> buffer);

		template<typename MESSAGE_PUBLISHER>
		void ConnectWriteSignal()
		{
			if (auto publisher = MESSAGE_PUBLISHER::GetPublisher())
			{
				m_WriteSignalConnections.push_back(publisher->connect(
					[this](typename MESSAGE_PUBLISHER::PublisherRef payload)
					{
						std::vector<uint8_t> buffer;
						if (payload.get().Serialize(buffer) && !buffer.empty())
						{
							EnqueueWrite(std::move(buffer));
						}
					}
				));
			}
		}

	private:
		void DrainWrites();
		void DrainReads();
		std::size_t ProcessMessages(ReadOps_SerialBufferType& circular_buffer);

	private:
		std::shared_ptr<Serial::SerialPort> m_SerialPort;
		std::shared_ptr<Kernel::StatisticsHub> m_StatisticsHub;

		std::deque<std::vector<uint8_t>> m_WriteQueue;

		ReadOps_SerialBufferType m_SerialBuffer;
		std::array<uint8_t, Constants::SERIAL_READ_CHUNK_SIZE> m_ReadBuffer{};
		std::size_t m_WriteOffset{0};

		// Capture-replay pacing period; zero = unpaced (see constructor).
		std::chrono::microseconds m_ReplayFramePeriod;

	private:
		std::vector<boost::signals2::scoped_connection> m_WriteSignalConnections;
	};

}
// namespace AqualinkAutomate::Protocol
