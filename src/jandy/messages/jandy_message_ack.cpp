#include <format>

#include <magic_enum/magic_enum.hpp>

#include "messages/jandy_message_ack.h"
#include "messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	JandyMessage_Ack::JandyMessage_Ack() noexcept :
		JandyMessage_Ack(AckTypes::Unknown, 0x00)
	{
	}

	JandyMessage_Ack::JandyMessage_Ack(AckTypes ack_type, uint8_t command) :
		JandyMessage_Ack(magic_enum::enum_integer(ack_type), command)
	{
	}

	JandyMessage_Ack::JandyMessage_Ack(uint8_t ack_value, uint8_t command) :
		JandyMessage(JandyMessageIds::Ack),
		Interfaces::IMessageSignalRecv<JandyMessage_Ack>(),
		Interfaces::IMessageSignalSend<JandyMessage_Ack>(),
		m_AckType(ack_value),
		m_Command(command)
	{
		m_Destination = Devices::JandyDeviceType(AQUALINK_MASTER_ID);
	}

	JandyMessage_Ack::~JandyMessage_Ack() = default;

	AckTypes JandyMessage_Ack::AckType() const
	{
		return magic_enum::enum_cast<AckTypes>(m_AckType).value_or(AckTypes::Unknown);
	}

	uint8_t JandyMessage_Ack::Command() const
	{
		return m_Command;
	}

	std::string JandyMessage_Ack::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	bool JandyMessage_Ack::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		message_bytes.emplace_back(m_AckType);
		message_bytes.emplace_back(m_Command);

		return true;
	}

	bool JandyMessage_Ack::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyMessage_Ack type", message_bytes.size()));

		if (message_bytes.size() <= Index_AckType)
		{
			LogDebug(Channel::Messages, "JandyMessage_Ack is too short to deserialise AckType.");
		}
		else if (message_bytes.size() <= Index_Command)
		{
			LogDebug(Channel::Messages, "JandyMessage_Ack is too short to deserialise Command.");
		}
		else
		{
			m_AckType = static_cast<uint8_t>(message_bytes[Index_AckType]);
			m_Command = static_cast<uint8_t>(message_bytes[Index_Command]);

			LogDebug(Channel::Messages, std::format("Deserialised JandyMessage_Ack: Ack Type -> {} (0x{:02x}), Command -> 0x{:02x}", m_AckType, static_cast<uint8_t>(message_bytes[Index_AckType]), m_Command));

			return true;
		}

		return false;
	}

}
// namespace AqualinkAutomate::Messages
