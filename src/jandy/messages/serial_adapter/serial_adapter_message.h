#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "jandy/messages/jandy_message.h"
#include "jandy/messages/jandy_message_ids.h"

namespace AqualinkAutomate::Messages
{
	class SerialAdapterMessage : public JandyMessage
	{
	public:
		SerialAdapterMessage(const JandyMessageIds& msg_id);
		virtual ~SerialAdapterMessage();

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override = 0;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override = 0;
	};

}
// namespace AqualinkAutomate::Messages