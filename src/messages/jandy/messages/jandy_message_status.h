#pragma once

#include <cstddef>
#include <string>
#include <span>

#include "messages/jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Messages::Jandy::Messages
{

	class JandyStatusMessage : public JandyMessage
	{
	public:
		JandyStatusMessage();
		virtual ~JandyStatusMessage();

	public:
		virtual std::string Print() const override;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const override;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes) override;
	};

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
