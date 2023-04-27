#include <format>

#include <magic_enum.hpp>

#include "jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Messages
{

	JandyMessage::JandyMessage(const JandyMessageIds& msg_id) :
		Interfaces::IMessage<JandyMessageIds>(msg_id),
		Interfaces::ISerializable(),
		m_Destination(), 
		m_RawId(0),
		m_MessageLength(0),
		m_ChecksumValue(0)
	{
	}

	JandyMessage::~JandyMessage()
	{
	}

	const Devices::JandyDeviceType JandyMessage::Destination() const
	{
		return m_Destination;
	}

	const uint8_t JandyMessage::RawId() const
	{
		return m_RawId;
	}

	const uint8_t JandyMessage::MessageLength() const
	{
		return m_MessageLength;
	}

	const uint8_t JandyMessage::ChecksumValue() const
	{
		return m_ChecksumValue;
	}

	std::string JandyMessage::ToString() const
	{
		return std::format(
			"Destination: {} (0x{:02x}), Message Type: {} (0x{:02x})", 
			magic_enum::enum_name(Destination().Class()), 
			Destination().Raw(), 
			magic_enum::enum_name(Id()),
			RawId()
		);
	}

	void JandyMessage::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void JandyMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			m_Destination = std::move(Devices::JandyDeviceType(static_cast<uint8_t>(message_bytes[Index_DestinationId])));
			m_RawId = static_cast<uint8_t>(message_bytes[Index_MessageType]);
			m_MessageLength = message_bytes.size_bytes();
			m_ChecksumValue = static_cast<uint8_t>(message_bytes[m_MessageLength - 3]);
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
