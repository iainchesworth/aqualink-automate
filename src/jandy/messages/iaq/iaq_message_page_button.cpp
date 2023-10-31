#include <algorithm>
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

	bool IAQMessage_PageButton::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool IAQMessage_PageButton::DeserializeContents(const std::vector<uint8_t>& message_bytes)
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
		else if (static_cast<uint64_t>(JandyMessage::MINIMUM_PACKET_LENGTH + 1 + 1 + 1) > message_bytes.size())
		{
			LogDebug(Channel::Messages, "IAQMessage_PageButton is too short to deserialise content of LineText");
		}
		else
		{
			m_ButtonStatus = magic_enum::enum_cast<ButtonStatuses>(static_cast<uint8_t>(message_bytes[Index_ButtonType])).value_or(ButtonStatuses::Unknown);
			m_ButtonType = magic_enum::enum_cast<ButtonTypes>(static_cast<uint8_t>(message_bytes[Index_ButtonType])).value_or(ButtonTypes::Unknown);

			const auto length_to_copy = message_bytes.size() - Index_ButtonNameText - 3;
			const auto start_index = message_bytes.begin() + Index_ButtonNameText;
			const auto end_index = start_index + length_to_copy;

			std::transform(start_index, end_index, std::back_inserter(m_ButtonName),
				[](const auto& elem)
				{
					return static_cast<char>(elem);
				}
			);

			return true;
		}

		return false;
	}

}
// namespace AqualinkAutomate::Messages
