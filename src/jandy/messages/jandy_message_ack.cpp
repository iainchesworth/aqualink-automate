#include <format>

#include <magic_enum.hpp>

#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_constants.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/utility/jandy_checksum.h"
#include "logging/logging.h"
#include "utility/array_standard_formatter.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::JandyMessage_Ack> JandyMessage_Ack::g_JandyMessage_Ack_Registration(JandyMessageIds::Ack);

	JandyMessage_Ack::JandyMessage_Ack() :
		JandyMessage_Ack(AckTypes::V1_Normal, 0x00)
	{
	}

	JandyMessage_Ack::JandyMessage_Ack(AckTypes ack_type, uint8_t command) :
		JandyMessage(JandyMessageIds::Ack),
		Interfaces::IMessageSignalRecv<JandyMessage_Ack>(),
		Interfaces::IMessageSignalSend<JandyMessage_Ack>(),
		m_AckType(ack_type),
		m_Command(command)
	{
		m_Destination = Devices::JandyDeviceType(AQUALINK_MASTER_ID);
	}

	JandyMessage_Ack::~JandyMessage_Ack()
	{
	}

	AckTypes JandyMessage_Ack::AckType() const
	{
		return m_AckType;
	}

	uint8_t JandyMessage_Ack::Command() const
	{
		return m_Command;
	}

	std::string JandyMessage_Ack::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	void JandyMessage_Ack::Serialize(std::vector<uint8_t>& message_bytes) const
	{
		message_bytes =
		{
			Messages::HEADER_BYTE_DLE,
			Messages::HEADER_BYTE_STX,
			0x00,
			magic_enum::enum_integer(JandyMessageIds::Ack),
			magic_enum::enum_integer(m_AckType),
			m_Command,
			0x00,
			Messages::HEADER_BYTE_DLE, 
			Messages::HEADER_BYTE_ETX
		};

		auto message_span_to_checksum = std::as_bytes(std::span<uint8_t>(message_bytes.begin(), 6));
		message_bytes[6] = Utility::JandyPacket_CalculateChecksum(message_span_to_checksum);

		LogTrace(Channel::Messages, std::format("Ack Message (Raw): {:.{}}", message_bytes, message_bytes.size()));
	}

	void JandyMessage_Ack::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyMessage_Ack type", message_bytes.size()));

			if (message_bytes.size() < Index_AckType)
			{
				LogDebug(Channel::Messages, "JandyMessage_Ack is too short to deserialise AckType.");
			}
			else if (message_bytes.size() < Index_Command)
			{
				LogDebug(Channel::Messages, "JandyMessage_Ack is too short to deserialise Command.");
			}
			else
			{
				m_AckType = magic_enum::enum_cast<AckTypes>(static_cast<uint8_t>(message_bytes[Index_AckType])).value_or(AckTypes::Unknown);
				m_Command = static_cast<uint8_t>(message_bytes[Index_Command]);

				LogDebug(Channel::Messages, std::format("Deserialised JandyMessage_Ack: Ack Type -> {} (0x{:02x}), Command -> 0x{:02x}", magic_enum::enum_name(m_AckType), static_cast<uint8_t>(message_bytes[Index_AckType]), m_Command));
			}

			JandyMessage::Deserialize(message_bytes);

			LogDebug(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
