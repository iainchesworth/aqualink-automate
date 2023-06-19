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

	class IAQMessage_PageEnd : public IAQMessage, public Interfaces::IMessageSignalRecv<IAQMessage_PageEnd>
	{
	public:
		IAQMessage_PageEnd();
		virtual ~IAQMessage_PageEnd();

	public:
		virtual std::string ToString() const;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;

	private:
		static const Factory::JandyMessageRegistration<Messages::IAQMessage_PageEnd> g_IAQMessage_PageEnd_Registration;
	};

}
// namespace AqualinkAutomate::Messages
