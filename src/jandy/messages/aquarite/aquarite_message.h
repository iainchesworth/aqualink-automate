#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "messages/jandy_message.h"
#include "messages/jandy_message_ids.h"

namespace AqualinkAutomate::Messages
{
	class AquariteMessage : public JandyMessage
	{
	public:
		AquariteMessage(const JandyMessageIds& msg_id);
		~AquariteMessage() override = default;

	public:
		std::string ToString() const override;

	public:
		bool SerializeContents(std::vector<uint8_t>& message_bytes) const override = 0;
		bool DeserializeContents(std::span<const uint8_t> message_bytes) override = 0;
	};

}
// namespace AqualinkAutomate::Messages
