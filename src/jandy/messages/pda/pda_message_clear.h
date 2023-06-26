#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/pda/pda_message.h"

namespace AqualinkAutomate::Messages
{

	class PDAMessage_Clear : public PDAMessage, public Interfaces::IMessageSignalRecv<PDAMessage_Clear>
	{
	public:
		PDAMessage_Clear();
		virtual ~PDAMessage_Clear();

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;

	private:
		static const Factory::JandyMessageRegistration<Messages::PDAMessage_Clear> g_PDAMessage_Clear_Registration;
	};

}
// namespace AqualinkAutomate::Messages
