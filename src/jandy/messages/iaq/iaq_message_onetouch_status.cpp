#include <format>

#include "messages/iaq/iaq_message_onetouch_status.h"
#include "messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage_OneTouchStatus::IAQMessage_OneTouchStatus() noexcept :
		IAQMessage(JandyMessageIds::IAQ_OneTouchStatus),
		Interfaces::IMessageSignalRecv<IAQMessage_OneTouchStatus>()
	{
	}

	IAQMessage_OneTouchStatus::~IAQMessage_OneTouchStatus()
	{
	}

	const std::vector<uint8_t>& IAQMessage_OneTouchStatus::RawPayload() const
	{
		return m_RawPayload;
	}

	std::string IAQMessage_OneTouchStatus::ToString() const
	{
		return std::format("Packet: {} || Payload: {} raw bytes", IAQMessage::ToString(), m_RawPayload.size());
	}

	bool IAQMessage_OneTouchStatus::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool IAQMessage_OneTouchStatus::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_OneTouchStatus type", message_bytes.size()));

		if (message_bytes.size() > 4 + 3)
		{
			m_RawPayload.assign(message_bytes.begin() + 4, message_bytes.end() - 3);
		}

		return true;
	}

}
// namespace AqualinkAutomate::Messages
