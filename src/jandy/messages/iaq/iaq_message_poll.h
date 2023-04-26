#pragma once

#include <cstddef>
#include <string>
#include <span>

#include "interfaces/imessagesignal.h"
#include "jandy/factories/jandy_message_factory_registration.h"
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

	private:
		static const Factory::JandyMessageRegistration<Messages::IAQMessage_Poll> g_IAQMessage_Poll_Registration;
	};

}
// namespace AqualinkAutomate::Messages
