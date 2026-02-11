#include <format>

#include "messages/iaq/iaq_message_main_status.h"
#include "messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage_MainStatus::IAQMessage_MainStatus() noexcept :
		IAQMessage(JandyMessageIds::IAQ_MainStatus),
		Interfaces::IMessageSignalRecv<IAQMessage_MainStatus>()
	{
	}

	IAQMessage_MainStatus::~IAQMessage_MainStatus() = default;

	const std::vector<uint8_t>& IAQMessage_MainStatus::RawPayload() const
	{
		return m_RawPayload;
	}

	std::string IAQMessage_MainStatus::ToString() const
	{
		return std::format("Packet: {} || Payload: {} raw bytes", IAQMessage::ToString(), m_RawPayload.size());
	}

	bool IAQMessage_MainStatus::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool IAQMessage_MainStatus::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_MainStatus type", message_bytes.size()));

		if (message_bytes.size() > 4 + 3)
		{
			m_RawPayload.assign(message_bytes.begin() + 4, message_bytes.end() - 3);
		}

		return true;
	}

}
// namespace AqualinkAutomate::Messages
