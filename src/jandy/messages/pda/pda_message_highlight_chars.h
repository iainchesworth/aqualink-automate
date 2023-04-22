#pragma once

#include <cstddef>
#include <string>
#include <span>

#include "interfaces/imessagesignal.h"
#include "jandy/messages/pda/pda_message.h"

namespace AqualinkAutomate::Messages
{

	class PDAMessage_HighlightChars : public PDAMessage, public Interfaces::IMessageSignal<PDAMessage_HighlightChars>
	{
	public:
		PDAMessage_HighlightChars();
		virtual ~PDAMessage_HighlightChars();

	public:
		virtual std::string ToString() const;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes);
	};

}
// namespace AqualinkAutomate::Messages
