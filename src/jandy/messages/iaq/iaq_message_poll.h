#pragma once

#include <cstddef>
#include <string>
#include <span>

#include "interfaces/imessagesignal.h"
#include "jandy/messages/iaq/iaq_message.h"

namespace AqualinkAutomate::Messages
{

	class IAQMessage_Poll : public IAQMessage, public Interfaces::IMessageSignal<IAQMessage_Poll>
	{
	public:
		IAQMessage_Poll();
		virtual ~IAQMessage_Poll();

	public:
		virtual std::string ToString() const;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes);
	};

}
// namespace AqualinkAutomate::Messages
