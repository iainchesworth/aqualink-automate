#include <functional>

#include "logging/logging.h"
#include "jandy/devices/keypad_device.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	KeypadDevice::KeypadDevice(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id) :
		JandyDevice(io_context, device_id, KEYPAD_TIMEOUT_DURATION)
	{
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_Message>(std::bind(&KeypadDevice::Slot_Keypad_Message, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_MessageLong>(std::bind(&KeypadDevice::Slot_Keypad_MessageLong, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_Status>(std::bind(&KeypadDevice::Slot_Keypad_Status, this, std::placeholders::_1), device_id());
	}

	KeypadDevice::~KeypadDevice()
	{
	}

	void KeypadDevice::Slot_Keypad_Message(const Messages::JandyMessage_Message& msg)
	{
		LogDebug(Channel::Devices, "RS Keypad device received a JandyMessage_Message signal.");

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void KeypadDevice::Slot_Keypad_MessageLong(const Messages::JandyMessage_MessageLong& msg)
	{
		LogDebug(Channel::Devices, "RS Keypad device received a JandyMessage_MessageLong signal.");

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void KeypadDevice::Slot_Keypad_Status(const Messages::JandyMessage_Status& msg)
	{
		LogDebug(Channel::Devices, "RS Keypad device received a JandyMessage_Status signal.");

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

}
// namespace AqualinkAutomate::Devices
