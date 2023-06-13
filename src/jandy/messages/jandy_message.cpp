#include <format>
#include <ranges>
#include <vector>

#include <magic_enum.hpp>

#include "jandy/formatters/jandy_device_formatters.h"
#include "jandy/messages/jandy_message.h"
#include "jandy/messages/jandy_message_constants.h"
#include "jandy/utility/jandy_checksum.h"
#include "jandy/utility/jandy_null_handler.h"
#include "logging/logging.h"
#include "utility/array_standard_formatter.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	JandyMessage::JandyMessage(const JandyMessageIds& msg_id) :
		Interfaces::IMessage<JandyMessageIds>(msg_id),
		Interfaces::ISerializable(),
		m_Destination(0x00), 
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

	uint8_t JandyMessage::MaxPermittedPacketLength() const
	{
		return MAXIMUM_PACKET_LENGTH;
	}

	uint8_t JandyMessage::MinPermittedPacketLength() const
	{
		return MINIMUM_PACKET_LENGTH;
	}

	std::string JandyMessage::ToString() const
	{
		return std::format(
			"Destination: {} ({}), Message Type: {} (0x{:02x})", 
			magic_enum::enum_name(Destination().Class()), 
			Destination().Id(), 
			magic_enum::enum_name(Id()),
			RawId()
		);
	}

	bool JandyMessage::Serialize(std::vector<uint8_t>& message_bytes) const
	{
		message_bytes.emplace_back(Messages::HEADER_BYTE_DLE);
		message_bytes.emplace_back(Messages::HEADER_BYTE_STX);
		message_bytes.emplace_back(JandyMessage::m_Destination.Id()());
		message_bytes.emplace_back(magic_enum::enum_integer(IMessage::Id()));

		SerializeContents(message_bytes);

		const auto message_span_to_checksum = std::as_bytes(std::span<uint8_t>(message_bytes.begin(), message_bytes.size()));

		message_bytes.emplace_back(Utility::JandyPacket_CalculateChecksum(message_span_to_checksum));
		message_bytes.emplace_back(Messages::HEADER_BYTE_DLE);
		message_bytes.emplace_back(Messages::HEADER_BYTE_ETX);
		
		Utility::JandyPacket_NullCharHandler_Serialization(message_bytes);

		LogTrace(Channel::Messages, std::format("{} Message (Raw): {:.{}}", magic_enum::enum_name(IMessage::Id()), message_bytes, message_bytes.size()));

		return true;
	}

	bool JandyMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (!PacketIsValid(message_bytes))
		{
			LogDebug(Channel::Messages, "Cannot deserialise JandyMessage packet; packet is not valid");
		}
		else
		{		
			std::vector<uint8_t> filtered_data(message_bytes.size());
			std::transform(message_bytes.begin(), message_bytes.end(), filtered_data.begin(),
				[](std::byte b)
				{
					return static_cast<uint8_t>(b);
				}
			);

			Utility::JandyPacket_NullCharHandler_Deserialization(filtered_data);

			m_Destination = std::move(Devices::JandyDeviceType(static_cast<uint8_t>(filtered_data[Index_DestinationId])));
			m_RawId = static_cast<uint8_t>(filtered_data[Index_MessageType]);
			m_MessageLength = message_bytes.size_bytes();
			m_ChecksumValue = static_cast<uint8_t>(filtered_data[m_MessageLength - PACKET_FOOTER_LENGTH]);

			return DeserializeContents(filtered_data);
		}

		return false;
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
