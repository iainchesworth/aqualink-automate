#pragma once

#include <cstddef>
#include <string>
#include <span>

#include "jandy/messages/iaq/iaq_message.h"

namespace AqualinkAutomate::Messages
{

	class IAQMessage_StartUp : public IAQMessage
	{
	public:
		IAQMessage_StartUp();
		virtual ~IAQMessage_StartUp();

	public:
		virtual std::string ToString() const;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes);
	};

}
// namespace AqualinkAutomate::Messages
