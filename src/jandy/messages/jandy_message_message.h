#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Messages
{

	class JandyMessage_Message : public JandyMessage, public Interfaces::IMessageSignalRecv<JandyMessage_Message>
	{
		static const uint8_t MAXIMUM_MESSAGE_LENGTH = 16 + 1; // 16 characters and a NUL terminator character.

		static const uint8_t Index_LineText = 4;

	public:
		JandyMessage_Message();
		JandyMessage_Message(const std::string& line);
		virtual ~JandyMessage_Message();

	public:
		std::string Line() const;

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;

	private:
		std::string m_Line;

	private:
		static const Factory::JandyMessageRegistration<Messages::JandyMessage_Message> g_JandyMessage_Message_Registration;
	};

}
// namespace AqualinkAutomate::Messages
