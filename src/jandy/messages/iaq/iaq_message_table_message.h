#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "messages/iaq/iaq_message.h"

namespace AqualinkAutomate::Messages
{

	class IAQMessage_TableMessage : public IAQMessage, public Interfaces::IMessageSignalRecv<IAQMessage_TableMessage>
	{
	public:
		static const uint8_t Index_LineId = 4;
		static const uint8_t Index_LineText = 5;

	public:
		IAQMessage_TableMessage() noexcept;
		virtual ~IAQMessage_TableMessage();

	public:
		uint8_t LineId() const;
		std::string Line() const;

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(std::span<const uint8_t> message_bytes) override;

	private:
		uint8_t m_LineId;
		std::string m_Line;
	};

}
// namespace AqualinkAutomate::Messages
