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

	class IAQMessage_PageContinue : public IAQMessage, public Interfaces::IMessageSignalRecv<IAQMessage_PageContinue>
	{
	public:
		IAQMessage_PageContinue();
		virtual ~IAQMessage_PageContinue();

	public:
		virtual std::string ToString() const;

	public:
		virtual void Serialize(std::vector<uint8_t>& message_bytes) const;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes);

	private:
		static const Factory::JandyMessageRegistration<Messages::IAQMessage_PageContinue> g_IAQMessage_PageContinue_Registration;
	};

}
// namespace AqualinkAutomate::Messages
