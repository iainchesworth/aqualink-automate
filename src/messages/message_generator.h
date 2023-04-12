#pragma once

#include <cstdint>
#include <expected>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

#include <boost/asio/awaitable.hpp>

#include "logging/logging.h"
#include "messages/message.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{
	template<typename MESSAGE_TYPE = std::shared_ptr<AqualinkAutomate::Messages::Message>, typename ERROR_TYPE = uint32_t>
	class MessageGenerator
	{
	public:
		typedef MESSAGE_TYPE MessageType;
		typedef ERROR_TYPE ErrorType;

	public:
		MessageGenerator() : m_SerialData(), m_SerialDataMutex() {}
		virtual ~MessageGenerator() {};

	public:
		void InjectRawSerialData(const std::span<const uint8_t>& read_buffer)
		{
			// Step 1 -> push the new bytes into the internal buffer.
			LogTrace(Channel::Messages, std::format("Existing serial buffer contains {} elements, adding {} new elements.", m_SerialData.size(), read_buffer.size()));

			std::lock_guard<std::mutex> lock(m_SerialDataMutex);
			m_SerialData.insert(m_SerialData.end(), read_buffer.begin(), read_buffer.end());
		}

	public: 
		virtual boost::asio::awaitable<std::expected<MessageType, ErrorType>> GenerateMessageFromRawData() = 0;

	protected:
		std::vector<std::uint8_t> m_SerialData;
		std::mutex m_SerialDataMutex;
	};

}
// namespace AqualinkAutomate::Messages
