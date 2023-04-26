#pragma once

#include <cstddef>
#include <string>
#include <span>

#include "interfaces/imessagesignal.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/pda/pda_message.h"

namespace AqualinkAutomate::Messages
{

	class PDAMessage_Clear : public PDAMessage, public Interfaces::IMessageSignal<PDAMessage_Clear>
	{
	public:
		PDAMessage_Clear();
		virtual ~PDAMessage_Clear();

	public:
		virtual std::string ToString() const;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes);

	private:
		static const Factory::JandyMessageRegistration<Messages::PDAMessage_Clear> g_PDAMessage_Clear_Registration;
	};

}
// namespace AqualinkAutomate::Messages
