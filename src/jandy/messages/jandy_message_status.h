#pragma once

#include <cstddef>
#include <string>
#include <span>

#include "interfaces/imessagesignal.h"
#include "jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Messages
{

	class JandyStatusMessage : public JandyMessage, public Interfaces::IMessageSignal<JandyStatusMessage>
	{
	public:
		JandyStatusMessage();
		virtual ~JandyStatusMessage();

	public:
		virtual std::string ToString() const override;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const override;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes) override;
	};

}
// namespace AqualinkAutomate::Messages
