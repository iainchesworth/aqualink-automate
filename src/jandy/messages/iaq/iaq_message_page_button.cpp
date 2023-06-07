#include <format>

#include <magic_enum.hpp>

#include "jandy/messages/iaq/iaq_message_page_button.h"
#include "jandy/messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::IAQMessage_PageButton> IAQMessage_PageButton::g_IAQMessage_PageButton_Registration(JandyMessageIds::IAQ_PageButton);

	IAQMessage_PageButton::IAQMessage_PageButton() : 
		IAQMessage(JandyMessageIds::IAQ_PageButton),
		Interfaces::IMessageSignalRecv<IAQMessage_PageButton>(),
		m_ButtonStatus(ButtonStatuses::Unknown),
		m_ButtonType(ButtonTypes::Unknown),
		m_ButtonName()
	{
	}

	IAQMessage_PageButton::~IAQMessage_PageButton()
	{
	}

	ButtonStatuses IAQMessage_PageButton::ButtonStatus() const
	{
		return m_ButtonStatus;
	}

	ButtonTypes IAQMessage_PageButton::ButtonType() const
	{
		return m_ButtonType;
	}

	std::string IAQMessage_PageButton::ButtonName() const
	{
		return m_ButtonName;
	}

	std::string IAQMessage_PageButton::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	void IAQMessage_PageButton::Serialize(std::vector<uint8_t>& message_bytes) const
	{
	}

	void IAQMessage_PageButton::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_PageButton type", message_bytes.size()));

			if (message_bytes.size() < Index_ButtonState)
			{
				LogDebug(Channel::Messages, "IAQMessage_PageButton is too short to deserialise ButtonState.");
			}
			else if (message_bytes.size() < Index_ButtonType)
			{
				LogDebug(Channel::Messages, "IAQMessage_PageButton is too short to deserialise ButtonType.");
			}
			else if (message_bytes.size() < Index_ButtonNameText)
			{
				LogDebug(Channel::Messages, "IAQMessage_PageButton is too short to deserialise context of ButtonName.");
			}
			else
			{
				m_ButtonStatus = magic_enum::enum_cast<ButtonStatuses>(static_cast<uint8_t>(message_bytes[Index_ButtonType])).value_or(ButtonStatuses::Unknown);
				m_ButtonType = magic_enum::enum_cast<ButtonTypes>(static_cast<uint8_t>(message_bytes[Index_ButtonType])).value_or(ButtonTypes::Unknown);

				const auto length_to_copy = message_bytes.size() - Index_ButtonNameText - 3;
				for (auto& elem : message_bytes.subspan(Index_ButtonNameText, length_to_copy))
				{
					// Convert to char and push into the string.
					m_ButtonName.push_back(static_cast<char>(elem));
				}
			}

			IAQMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
