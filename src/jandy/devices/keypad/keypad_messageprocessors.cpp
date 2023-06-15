#include <magic_enum.hpp>

#include "logging/logging.h"
#include "jandy/devices/keypad_device.h"
#include "jandy/formatters/screen_data_page_formatter.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{
	void KeypadDevice::Slot_Keypad_Ack(const Messages::JandyMessage_Ack& msg)
	{
		LogDebug(Channel::Devices, "RS Keypad device received a JandyMessage_Ack signal.");

		ProcessControllerUpdates();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void KeypadDevice::Slot_Keypad_Probe(const Messages::JandyMessage_Probe& msg)
	{
		LogDebug(Channel::Devices, "RS Keypad device received a JandyMessage_Probe signal.");

		Signal_AckMessage(Messages::AckTypes::V2_Normal, KeyCommands::NoKeyCommand);
		ProcessControllerUpdates();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void KeypadDevice::Slot_Keypad_Message(const Messages::JandyMessage_Message& msg)
	{
		LogDebug(Channel::Devices, "RS Keypad device received a JandyMessage_Message signal.");

		Signal_AckMessage(Messages::AckTypes::V2_Normal, KeyCommands::NoKeyCommand);
		ProcessControllerUpdates();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void KeypadDevice::Slot_Keypad_MessageLong(const Messages::JandyMessage_MessageLong& msg)
	{
		LogDebug(Channel::Devices, "RS Keypad device received a JandyMessage_MessageLong signal.");

		Signal_AckMessage(Messages::AckTypes::V2_Normal, KeyCommands::NoKeyCommand);
		ProcessControllerUpdates();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void KeypadDevice::Slot_Keypad_Status(const Messages::JandyMessage_Status& msg)
	{
		LogDebug(Channel::Devices, "RS Keypad device received a JandyMessage_Status signal.");

		Signal_AckMessage(Messages::AckTypes::V2_Normal, KeyCommands::NoKeyCommand);
		ProcessControllerUpdates();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

}
// namespace AqualinkAutomate::Devices
