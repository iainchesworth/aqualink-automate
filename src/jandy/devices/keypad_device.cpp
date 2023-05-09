#include <boost/bind/bind.hpp>

#include "logging/logging.h"
#include "jandy/devices/keypad_device.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	KeypadDevice::KeypadDevice(boost::asio::io_context& io_context, IDevice::DeviceId id) :
		IDevice(io_context, id, KEYPAD_TIMEOUT_DURATION)
	{

		Messages::JandyMessage_Message::GetSignal()->connect(boost::bind(&KeypadDevice::Slot_Keypad_Message, this, boost::placeholders::_1));
		Messages::JandyMessage_MessageLong::GetSignal()->connect(boost::bind(&KeypadDevice::Slot_Keypad_MessageLong, this, boost::placeholders::_1));
		Messages::JandyMessage_Status::GetSignal()->connect(boost::bind(&KeypadDevice::Slot_Keypad_Status, this, boost::placeholders::_1));
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
