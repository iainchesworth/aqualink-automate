#include <format>

#include "messages/epump/epump_message_watts.h"
#include "messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	EPumpMessage_Watts::EPumpMessage_Watts() noexcept :
		EPumpMessage(JandyMessageIds::EPUMP_Watts),
		Interfaces::IMessageSignalRecv<EPumpMessage_Watts>(),
		m_Watts(0)
	{
	}


	uint16_t EPumpMessage_Watts::Watts() const
	{
		return m_Watts;
	}

	std::string EPumpMessage_Watts::ToString() const
	{
		return std::format("Packet: {} || Payload: Watts: {}", EPumpMessage::ToString(), m_Watts);
	}

	bool EPumpMessage_Watts::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool EPumpMessage_Watts::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into EPumpMessage_Watts type", message_bytes.size()));

		if (message_bytes.size() <= Index_Watts_High)
		{
			LogDebug(Channel::Messages, "EPumpMessage_Watts is too short to deserialise Watts.");
		}
		else
		{
			const uint8_t low = static_cast<uint8_t>(message_bytes[Index_Watts_Low]);
			const uint8_t high = static_cast<uint8_t>(message_bytes[Index_Watts_High]);
			m_Watts = static_cast<uint16_t>(high * 256 + low);
			return true;
		}

		return false;
	}

}
// namespace AqualinkAutomate::Messages
