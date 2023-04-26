#pragma once

#include <cstddef>
#include <string>
#include <span>

#include "interfaces/imessagesignal.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Messages
{

	class JandyMessage_MessageLong : public JandyMessage, public Interfaces::IMessageSignal<JandyMessage_MessageLong>
	{
	public:
		JandyMessage_MessageLong();
		virtual ~JandyMessage_MessageLong();

	public:
		virtual std::string ToString() const override;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const override;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes) override;

	private:
		static const Factory::JandyMessageRegistration<Messages::JandyMessage_MessageLong> g_JandyMessage_MessageLong_Registration;
	};

}
// namespace AqualinkAutomate::Messages
