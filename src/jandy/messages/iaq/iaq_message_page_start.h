#pragma once

#include <cstddef>
#include <string>
#include <span>

#include "jandy/messages/iaq/iaq_message.h"

namespace AqualinkAutomate::Messages
{

	class IAQMessage_PageStart : public IAQMessage
	{
	public:
		IAQMessage_PageStart();
		virtual ~IAQMessage_PageStart();

	public:
		virtual std::string ToString() const;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes);
	};

}
// namespace AqualinkAutomate::Messages
