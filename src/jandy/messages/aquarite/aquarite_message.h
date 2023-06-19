#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "jandy/messages/jandy_message.h"
#include "jandy/messages/jandy_message_ids.h"

namespace AqualinkAutomate::Messages
{
	class AquariteMessage : public JandyMessage
	{
	public:
		AquariteMessage(const JandyMessageIds& msg_id);
		virtual ~AquariteMessage();

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const = 0;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) = 0;
	};

}
// namespace AqualinkAutomate::Messages
