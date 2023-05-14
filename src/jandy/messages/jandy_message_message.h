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
	public:
		JandyMessage_Message();
		virtual ~JandyMessage_Message();

	public:
		virtual std::string ToString() const override;

	public:
		virtual void Serialize(std::vector<uint8_t>& message_bytes) const override;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes) override;

	private:
		static const Factory::JandyMessageRegistration<Messages::JandyMessage_Message> g_JandyMessage_Message_Registration;
	};

}
// namespace AqualinkAutomate::Messages
