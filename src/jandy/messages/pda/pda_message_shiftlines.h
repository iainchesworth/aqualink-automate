#pragma once

#include <cstddef>
#include <string>
#include <span>

#include "interfaces/imessagesignal.h"
#include "jandy/messages/pda/pda_message.h"

namespace AqualinkAutomate::Messages
{

	class PDAMessage_ShiftLines : public PDAMessage, public Interfaces::IMessageSignal<PDAMessage_ShiftLines>
	{
	public:
		PDAMessage_ShiftLines();
		virtual ~PDAMessage_ShiftLines();

	public:
		virtual std::string ToString() const;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes);
	};

}
// namespace AqualinkAutomate::Messages
