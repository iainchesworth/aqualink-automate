#pragma once

#include <cstdint>
#include <format>
#include <mutex>
#include <vector>

#include <boost/signals2.hpp>

#include "logging/logging.h"
#include "profiling/profiling.h"
#include "serial/serial_port.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Protocol
{

	class ProtocolHandler
	{
	public:
		ProtocolHandler(boost::asio::io_context& io_context, Serial::SerialPort& serial_port);
		~ProtocolHandler();

	public:
		template<typename MESSAGE_PUBLISHER>
		void RegisterPublishableMessage()
		{
			auto publish_message = [&](const auto& msg) -> bool
			{
				std::vector<uint8_t> buffer;
				if (!msg.Serialize(buffer))
				{
					LogTrace(Channel::Protocol, "Failed to serialise message during publishing");
				}
				else if (!PublishRawData(std::move(buffer)))
				{
					LogTrace(Channel::Protocol, "Failed to move serialised message buffer during publishing");
				}
				else
				{
					return true;
				}

				return false;
			};

			m_PublishableMessages.push_back(MESSAGE_PUBLISHER::GetPublisher()->connect(publish_message));
		}

		bool PublishRawData(std::vector<uint8_t>&& raw_data)
		{
			std::lock_guard<std::mutex> lock(m_SerialData_OutgoingMutex);
			std::move(raw_data.begin(), raw_data.end(), std::back_inserter(m_SerialData_Outgoing));

			return true;
		}

	public:
		void Run();

	private:
		bool HandleRead();
		bool HandleRead_Success(auto& read_buffer, auto bytes_read);
		bool HandleWrite();
		bool HandleWrite_Success(auto& write_buffer, auto bytes_written);
		bool HandleWrite_Partial(auto& write_buffer, auto bytes_written);

	private:
		Serial::SerialPort& m_SerialPort;
		std::vector<uint8_t> m_SerialData_Incoming;
		std::vector<uint8_t> m_SerialData_Outgoing;
		std::mutex m_SerialData_IncomingMutex;
		std::mutex m_SerialData_OutgoingMutex;

	private:
		std::vector<boost::signals2::connection> m_PublishableMessages;

	private:
		Types::ProfilingUnitTypePtr m_ProfilingDomain;
	};
}
// namespace AqualinkAutomate::Protocol
