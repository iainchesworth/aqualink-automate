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

	class PDAMessage_HighlightChars : public PDAMessage, public Interfaces::IMessageSignalRecv<PDAMessage_HighlightChars>
	{
	public:
		PDAMessage_HighlightChars();
		virtual ~PDAMessage_HighlightChars();

	public:
		virtual std::string ToString() const;

	public:
		virtual void Serialize(std::vector<uint8_t>& message_bytes) const;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes);

	private:
		static const Factory::JandyMessageRegistration<Messages::PDAMessage_HighlightChars> g_PDAMessage_HighlightChars_Registration;
	};

}
// namespace AqualinkAutomate::Messages
