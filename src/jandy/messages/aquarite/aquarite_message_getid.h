#pragma once

#include <cstddef>
#include <string>
#include <span>

#include "interfaces/imessagesignal.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/aquarite/aquarite_message.h"

namespace AqualinkAutomate::Messages
{

	class AquariteMessage_GetId : public AquariteMessage, public Interfaces::IMessageSignal<AquariteMessage_GetId>
	{
	public:
		AquariteMessage_GetId();
		virtual ~AquariteMessage_GetId();

	public:
		virtual std::string ToString() const;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes);

	private:
		static const Factory::JandyMessageRegistration<Messages::AquariteMessage_GetId> g_AquariteMessage_GetId_Registration;
	};

}
// namespace AqualinkAutomate::Messages
