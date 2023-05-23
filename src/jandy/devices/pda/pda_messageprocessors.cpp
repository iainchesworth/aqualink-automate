#include <cmath>
#include <format>

#include <magic_enum.hpp>

#include "logging/logging.h"
#include "jandy/devices/pda_device.h"
#include "jandy/messages/jandy_message_ack.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	void PDADevice::Slot_PDA_Ack(const Messages::JandyMessage_Ack& msg)
	{
		LogDebug(Channel::Devices, "PDA device received a JandyMessage_Ack signal.");
	}

	void PDADevice::Slot_PDA_Clear(const Messages::PDAMessage_Clear& msg)
	{
		LogDebug(Channel::Devices, "PDA device received a JandyMessage_Clear signal.");
	}

	void PDADevice::Slot_PDA_Highlight(const Messages::PDAMessage_Highlight& msg)
	{
		LogDebug(Channel::Devices, "PDA device received a PDAMessage_Highlight signal.");
	}

	void PDADevice::Slot_PDA_HighlightChars(const Messages::PDAMessage_HighlightChars& msg)
	{
		LogDebug(Channel::Devices, "PDA device received a PDAMessage_HighlightChars signal.");
	}

	void PDADevice::Slot_PDA_MessageLong(const Messages::JandyMessage_MessageLong& msg)
	{
		LogDebug(Channel::Devices, "PDA device received a JandyMessage_MessageLong signal.");
	}

	void PDADevice::Slot_PDA_Status(const Messages::JandyMessage_Status& msg)
	{
		LogDebug(Channel::Devices, "PDA device received a JandyMessage_Status signal.");
	}

	void PDADevice::Slot_PDA_ShiftLines(const Messages::PDAMessage_ShiftLines& msg)
	{
		LogDebug(Channel::Devices, "PDA device received a PDAMessage_ShiftLines signal.");
	}

	void PDADevice::Slot_PDA_Unknown_PDA_1B(const Messages::JandyMessage_Unknown& msg)
	{
		LogDebug(Channel::Devices, "PDA device received a JandyMessage_Unknown signal.");
	}
	
	void PDADevice::Signal_PDA_AckMessage() const
	{
		if (JandyControllerOperatingModes::Emulated == m_OpMode)
		{
			LogDebug(Channel::Devices, "Emulated OneTouch device sending ACKnowledgement message");

			auto ack_message = std::make_shared<Messages::JandyMessage_Ack>(Messages::AckTypes::V2_Normal, 0x00);
			ack_message->Signal_MessageToSend();
		}
	}

}
// namespace AqualinkAutomate::Devices
