#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/iaq/iaq_message.h"

namespace AqualinkAutomate::Messages
{

	class IAQMessage_TableMessage : public IAQMessage, public Interfaces::IMessageSignalRecv<IAQMessage_TableMessage>
	{
	public:
		static const uint8_t Index_LineId = 4;
		static const uint8_t Index_LineText = 5;

	public:
		IAQMessage_TableMessage();
		virtual ~IAQMessage_TableMessage();

	public:
		uint8_t LineId() const;
		std::string Line() const;

	public:
		virtual std::string ToString() const;

	public:
		virtual void Serialize(std::vector<uint8_t>& message_bytes) const;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes);

	private:
		uint8_t m_LineId;
		std::string m_Line;

	private:
		static const Factory::JandyMessageRegistration<Messages::IAQMessage_TableMessage> g_IAQMessage_TableMessage_Registration;
	};

}
// namespace AqualinkAutomate::Messages
