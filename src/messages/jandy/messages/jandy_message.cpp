#include <format>

#include "messages/jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Messages::Jandy::Messages
{

	JandyMessage::JandyMessage() : m_Destination(DefaultDestination)
	{
	}

	JandyMessage::~JandyMessage()
	{
	}

	const uint8_t JandyMessage::DestinationId() const
	{
		return m_Destination;
	}

	std::string JandyMessage::Print() const
	{
		return std::format("Destination: 0x{:02x}", DestinationId());
	}

	void JandyMessage::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void JandyMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if ((MINIMUM_PACKET_LENGTH > message_bytes.size()) || (MAXIMUM_PACKET_LENGTH < message_bytes.size()))
		{
			// LogDebug
		}
		else
		{
			m_Destination = static_cast<uint8_t>(message_bytes[Index_DestinationId]);
		}
	}

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
