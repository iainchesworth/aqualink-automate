#pragma once

#include <array>
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
		ProtocolTask(std::shared_ptr<Serial::SerialPort> serial_port,
					 std::shared_ptr<Kernel::StatisticsHub> statistics_hub);
		~ProtocolTask();

		ProtocolTask(const ProtocolTask&) = delete;
		ProtocolTask& operator=(const ProtocolTask&) = delete;
		ProtocolTask(ProtocolTask&&) = delete;
		ProtocolTask& operator=(ProtocolTask&&) = delete;

		void Poll();

		void EnqueueWrite(std::vector<uint8_t> buffer);

		template<typename MESSAGE_PUBLISHER>
		void ConnectWriteSignal()
		{
			if (auto publisher = MESSAGE_PUBLISHER::GetPublisher())
			{
				m_WriteSignalConnection = publisher->connect(
					[this](typename MESSAGE_PUBLISHER::PublisherRef payload)
					{
						std::vector<uint8_t> buffer;
						if (payload.get().Serialize(buffer) && !buffer.empty())
						{
							EnqueueWrite(std::move(buffer));
						}
					}
				);
			}
		}

	private:
		void ProcessMessages(ReadOps_SerialBufferType& circular_buffer);

	private:
		std::shared_ptr<Serial::SerialPort> m_SerialPort;
		std::shared_ptr<Kernel::StatisticsHub> m_StatisticsHub;

		std::deque<std::vector<uint8_t>> m_WriteQueue;

		ReadOps_SerialBufferType m_SerialBuffer;
		std::array<uint8_t, Constants::SERIAL_READ_CHUNK_SIZE> m_ReadBuffer{};

	private:
		boost::signals2::scoped_connection m_WriteSignalConnection;
	};

}
// namespace AqualinkAutomate::Protocol
