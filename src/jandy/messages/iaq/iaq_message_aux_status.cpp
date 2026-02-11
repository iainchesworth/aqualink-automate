#include <format>

#include "messages/iaq/iaq_message_aux_status.h"
#include "messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage_AuxStatus::IAQMessage_AuxStatus() noexcept :
		IAQMessage(JandyMessageIds::IAQ_AuxStatus),
		Interfaces::IMessageSignalRecv<IAQMessage_AuxStatus>()
	{
	}

	IAQMessage_AuxStatus::~IAQMessage_AuxStatus() = default;

	const std::vector<uint8_t>& IAQMessage_AuxStatus::RawPayload() const
	{
		return m_RawPayload;
	}

	std::string IAQMessage_AuxStatus::ToString() const
	{
		return std::format("Packet: {} || Payload: {} raw bytes", IAQMessage::ToString(), m_RawPayload.size());
	}

	bool IAQMessage_AuxStatus::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool IAQMessage_AuxStatus::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_AuxStatus type", message_bytes.size()));

		if (message_bytes.size() > 4 + 3)
		{
			m_RawPayload.assign(message_bytes.begin() + 4, message_bytes.end() - 3);
		}

		return true;
	}

}
// namespace AqualinkAutomate::Messages
