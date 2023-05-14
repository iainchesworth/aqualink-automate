#include <format>
#include <functional>

#include "logging/logging.h"
#include "jandy/devices/pda_device.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	PDADevice::PDADevice(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id) :
		JandyDevice(io_context, device_id, PDA_TIMEOUT_DURATION)
	{
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_Ack>(std::bind(&PDADevice::Slot_PDA_Ack, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::PDAMessage_Clear>(std::bind(&PDADevice::Slot_PDA_Clear, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::PDAMessage_Highlight>(std::bind(&PDADevice::Slot_PDA_Highlight, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::PDAMessage_HighlightChars>(std::bind(&PDADevice::Slot_PDA_HighlightChars, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_MessageLong>(std::bind(&PDADevice::Slot_PDA_MessageLong, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_Status>(std::bind(&PDADevice::Slot_PDA_Status, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::PDAMessage_ShiftLines>(std::bind(&PDADevice::Slot_PDA_ShiftLines, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_Unknown>(std::bind(&PDADevice::Slot_PDA_Unknown_PDA_1B, this, std::placeholders::_1), device_id());
	}

	PDADevice::~PDADevice()
	{
	}

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

}
// namespace AqualinkAutomate::Devices
