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

	class JandyMessage_MessageLoopStart : public JandyMessage, public Interfaces::IMessageSignalRecv<JandyMessage_MessageLoopStart>
	{
	public:
		JandyMessage_MessageLoopStart();
		virtual ~JandyMessage_MessageLoopStart();

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;

	private:
		static const Factory::JandyMessageRegistration<Messages::JandyMessage_MessageLoopStart> g_JandyMessage_MessageLoopStart_Registration;
	};

}
// namespace AqualinkAutomate::Messages
