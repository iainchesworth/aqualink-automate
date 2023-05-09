#include <format>
#include <ranges>
#include <vector>

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
		m_PayloadLength(0),
		m_Payload(),
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
			m_PayloadLength = m_MessageLength - MINIMUM_PACKET_LENGTH;
			m_ChecksumValue = static_cast<uint8_t>(message_bytes[m_MessageLength - PACKET_FOOTER_LENGTH]);

			auto payload_bytes = message_bytes.subspan(PACKET_HEADER_LENGTH, message_bytes.size() - MINIMUM_PACKET_LENGTH);

			bool last_byte_was_0x10 = false;

			std::vector filtered_data(payload_bytes.begin(), payload_bytes.end());
			std::erase_if(
				filtered_data,
				[&last_byte_was_0x10](std::byte current_byte) -> bool
				{
					bool should_remove = last_byte_was_0x10 && current_byte == std::byte{ 0x00 };
					last_byte_was_0x10 = current_byte == std::byte{ 0x10 };
					return should_remove;
				}
			);

			for (auto& elem : filtered_data)
			{
				m_Payload.push_back(static_cast<uint8_t>(elem));
			}
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
