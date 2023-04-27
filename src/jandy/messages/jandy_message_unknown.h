#pragma once

#include <cstddef>
#include <string>
#include <span>

#include "interfaces/imessagesignal.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Messages
{

	class JandyMessage_Unknown : public JandyMessage, public Interfaces::IMessageSignal<JandyMessage_Unknown>
	{
	public:
		JandyMessage_Unknown();
		virtual ~JandyMessage_Unknown();

	public:
		virtual std::string ToString() const override;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const override;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes) override;

	private:
		static const Factory::JandyMessageRegistration<Messages::JandyMessage_Unknown> g_JandyMessage_Unknown_PDA_1B_Registration;
		static const Factory::JandyMessageRegistration<Messages::JandyMessage_Unknown> g_JandyMessage_Unknown_IAQ_2D_Registration;
		static const Factory::JandyMessageRegistration<Messages::JandyMessage_Unknown> g_JandyMessage_Unknown_IAQ_70_Registration;
		static const Factory::JandyMessageRegistration<Messages::JandyMessage_Unknown> g_JandyMessage_Unknown_ReadyControl_Registration;
		static const Factory::JandyMessageRegistration<Messages::JandyMessage_Unknown> g_JandyMessage_Unknown_Registration;
	};

}
// namespace AqualinkAutomate::Messages
