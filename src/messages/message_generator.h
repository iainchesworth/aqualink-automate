#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <memory>
#include <vector>

#include "messages/message.h"

namespace AqualinkAutomate::Messages
{
	template<typename MESSAGE = std::shared_ptr<AqualinkAutomate::Messages::Message>>
	class MessageGenerator
	{
	public:
		typedef MESSAGE Message;

	public:
		MessageGenerator() : m_RawMessageBuffer(1024)
		{
		}

	public: 
		virtual std::expected<Message, uint32_t> GenerateMessageFromRawData(const std::array<uint8_t, 16>& read_buffer, std::size_t bytes_read) = 0;

	protected:
		std::vector<uint8_t> m_RawMessageBuffer;
	};

}
// namespace AqualinkAutomate::Messages
