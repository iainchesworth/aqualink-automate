#pragma once

#include <chrono>

#include "interfaces/idevice.h"
#include "jandy/messages/jandy_message_message.h"
#include "jandy/messages/jandy_message_message_long.h"
#include "jandy/messages/jandy_message_status.h"

namespace AqualinkAutomate::Devices
{

	class KeypadDevice : public Interfaces::IDevice
	{
		const std::chrono::seconds KEYPAD_TIMEOUT_DURATION = std::chrono::seconds(30);

	public:
		KeypadDevice(boost::asio::io_context& io_context, IDevice::DeviceId id);
		virtual ~KeypadDevice();

	private:
		void Slot_Keypad_Message(const Messages::JandyMessage_Message& msg);
		void Slot_Keypad_MessageLong(const Messages::JandyMessage_MessageLong& msg);
		void Slot_Keypad_Status(const Messages::JandyMessage_Status& msg);
	};

}
// namespace AqualinkAutomate::Devices
