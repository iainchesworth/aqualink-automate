#include <functional>

#include "logging/logging.h"
#include "jandy/devices/keypad_device.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	KeypadDevice::KeypadDevice(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, Config::JandyConfig& config, bool is_emulated) :
		JandyController(io_context, device_id, KEYPAD_TIMEOUT_DURATION, config),
		Capabilities::Screen(KEYPAD_PAGE_LINES),
		Capabilities::Emulated(is_emulated)
	{
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_Probe>(std::bind(&KeypadDevice::Slot_Keypad_Probe, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_Message>(std::bind(&KeypadDevice::Slot_Keypad_Message, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_MessageLong>(std::bind(&KeypadDevice::Slot_Keypad_MessageLong, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_MessageLoopStart>(std::bind(&KeypadDevice::Slot_Keypad_MessageLoopStart, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_Status>(std::bind(&KeypadDevice::Slot_Keypad_Status, this, std::placeholders::_1), device_id());

		if (!IsEmulated())
		{
			m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_Ack>(std::bind(&KeypadDevice::Slot_Keypad_Ack, this, std::placeholders::_1), device_id());
		}
	}
	
	KeypadDevice::~KeypadDevice()
	{
	}

	void KeypadDevice::ProcessControllerUpdates()
	{
	}

}
// namespace AqualinkAutomate::Devices
