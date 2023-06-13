#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Messages
{

	class JandyMessage_MessageLong : public JandyMessage, public Interfaces::IMessageSignalRecv<JandyMessage_MessageLong>
	{
		static const uint8_t MAXIMUM_MESSAGE_LENGTH = 16 + 1; // 16 characters and a NUL terminator character.

		static const uint8_t Index_LineId = 4;
		static const uint8_t Index_LineText = 5;

	public:
		JandyMessage_MessageLong();
		virtual ~JandyMessage_MessageLong();

	public:
		uint8_t LineId() const;
		std::string Line() const;

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;

	private:
		uint8_t m_LineId;
		std::string m_Line;

	private:
		static const Factory::JandyMessageRegistration<Messages::JandyMessage_MessageLong> g_JandyMessage_MessageLong_Registration;
	};

}
// namespace AqualinkAutomate::Messages
