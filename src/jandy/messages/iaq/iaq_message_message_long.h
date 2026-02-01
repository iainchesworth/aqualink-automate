#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "messages/iaq/iaq_message.h"

namespace AqualinkAutomate::Messages
{

	class IAQMessage_MessageLong : public IAQMessage, public Interfaces::IMessageSignalRecv<IAQMessage_MessageLong>
	{
	public:
		IAQMessage_MessageLong() noexcept;
		virtual ~IAQMessage_MessageLong();

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;
	};

}
// namespace AqualinkAutomate::Messages
