#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "messages/iaq/iaq_message.h"

namespace AqualinkAutomate::Messages
{

	class IAQMessage_Heartbeat : public IAQMessage, public Interfaces::IMessageSignalRecv<IAQMessage_Heartbeat>
	{
	public:
		IAQMessage_Heartbeat() noexcept;
		virtual ~IAQMessage_Heartbeat();

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(std::span<const uint8_t> message_bytes) override;
	};

}
// namespace AqualinkAutomate::Messages
