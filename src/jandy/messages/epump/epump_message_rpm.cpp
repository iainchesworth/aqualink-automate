#include <format>

#include "messages/epump/epump_message_rpm.h"
#include "messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	EPumpMessage_RPM::EPumpMessage_RPM() noexcept :
		EPumpMessage(JandyMessageIds::EPUMP_RPM),
		Interfaces::IMessageSignalRecv<EPumpMessage_RPM>(),
		m_RPM(0)
	{
	}


	uint16_t EPumpMessage_RPM::RPM() const
	{
		return m_RPM;
	}

	std::string EPumpMessage_RPM::ToString() const
	{
		return std::format("Packet: {} || Payload: RPM: {}", EPumpMessage::ToString(), m_RPM);
	}

	bool EPumpMessage_RPM::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool EPumpMessage_RPM::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into EPumpMessage_RPM type", message_bytes.size()));

		if (message_bytes.size() <= Index_RPM_High)
		{
			LogDebug(Channel::Messages, "EPumpMessage_RPM is too short to deserialise RPM.");
		}
		else
		{
			const uint8_t low = static_cast<uint8_t>(message_bytes[Index_RPM_Low]);
			const uint8_t high = static_cast<uint8_t>(message_bytes[Index_RPM_High]);
			m_RPM = static_cast<uint16_t>((high * 256 + low) / 4);
			return true;
		}

		return false;
	}

}
// namespace AqualinkAutomate::Messages
