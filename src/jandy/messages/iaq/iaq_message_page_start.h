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

	class IAQMessage_PageStart : public IAQMessage, public Interfaces::IMessageSignalRecv<IAQMessage_PageStart>
	{
	public:
		IAQMessage_PageStart();
		virtual ~IAQMessage_PageStart();

	public:
		virtual std::string ToString() const;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;

	private:
		static const Factory::JandyMessageRegistration<Messages::IAQMessage_PageStart> g_IAQMessage_PageStart_Registration;
	};

}
// namespace AqualinkAutomate::Messages
