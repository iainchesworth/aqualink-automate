#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "messages/iaq/iaq_message.h"

namespace AqualinkAutomate::Messages
{

	class IAQMessage_MainStatus : public IAQMessage, public Interfaces::IMessageSignalRecv<IAQMessage_MainStatus>
	{
	public:
		IAQMessage_MainStatus() noexcept;
		virtual ~IAQMessage_MainStatus();

	public:
		const std::vector<uint8_t>& RawPayload() const;

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(std::span<const uint8_t> message_bytes) override;

	private:
		std::vector<uint8_t> m_RawPayload;
	};

}
// namespace AqualinkAutomate::Messages
