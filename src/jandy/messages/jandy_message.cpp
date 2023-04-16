#include <format>

#include "jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Messages
{

	JandyMessage::JandyMessage(const JandyMessageIds& msg_id) :
		Interfaces::IMessage<JandyMessageIds>(msg_id),
		Interfaces::ISerializable(),
		m_Destination(DefaultDestination), 
		m_MessageType()
	{
	}

	JandyMessage::~JandyMessage()
	{
	}

	const uint8_t JandyMessage::DestinationId() const
	{
		return m_Destination;
	}

	const uint8_t JandyMessage::MessageType() const
	{
		return m_MessageType;
	}

	std::string JandyMessage::ToString() const
	{
		return std::format("Destination: 0x{:02x}, Message Type 0x{:02x}", DestinationId(), MessageType());
	}

	void JandyMessage::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void JandyMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			m_Destination = static_cast<uint8_t>(message_bytes[Index_DestinationId]);
			m_MessageType = static_cast<uint8_t>(message_bytes[Index_MessageType]);
		}
	}

	bool JandyMessage::PacketIsValid(const std::span<const std::byte>& message_bytes) const
	{
		bool packet_is_valid = true;

		packet_is_valid &= (MINIMUM_PACKET_LENGTH <= message_bytes.size());
		packet_is_valid &= (MAXIMUM_PACKET_LENGTH >= message_bytes.size());

		return packet_is_valid;
	}

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
