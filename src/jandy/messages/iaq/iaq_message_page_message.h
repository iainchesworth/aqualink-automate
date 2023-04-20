#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <span>

#include "interfaces/imessagesignal.h"
#include "jandy/messages/iaq/iaq_message.h"

namespace AqualinkAutomate::Messages
{

	class IAQMessage_PageMessage : public IAQMessage, public Interfaces::IMessageSignal<IAQMessage_PageMessage>
	{
	public:
		static const uint8_t Index_LineId = 4;
		static const uint8_t Index_LineText = 5;

	public:
		IAQMessage_PageMessage();
		virtual ~IAQMessage_PageMessage();

	public:
		uint8_t LineId() const;
		std::string Line() const;

	public:
		virtual std::string ToString() const;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes);

	private:
		uint8_t m_LineId;
		std::string m_Line;
	};

}
// namespace AqualinkAutomate::Messages
