#pragma once

#include <chrono>
#include <cstdint>

#include "jandy/config/jandy_config.h"
#include "jandy/devices/jandy_controller.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/devices/capabilities/emulated.h"
#include "jandy/devices/capabilities/scrapeable.h"
#include "jandy/devices/capabilities/screen.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_probe.h"
#include "jandy/messages/jandy_message_message.h"
#include "jandy/messages/jandy_message_message_long.h"
#include "jandy/messages/jandy_message_status.h"

namespace AqualinkAutomate::Devices
{

	class KeypadDevice : public JandyController, public Capabilities::Screen, public Capabilities::Emulated
	{
		inline static const uint8_t KEYPAD_PAGE_LINES{ 3 };
		inline static const std::chrono::seconds KEYPAD_TIMEOUT_DURATION{ std::chrono::seconds(30) };

		enum class KeyCommands
		{
			NoKeyCommand = 0x00,
		};

	public:
		KeypadDevice(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, Config::JandyConfig& config, bool is_emulated);
		virtual ~KeypadDevice();

	private:
		virtual void ProcessControllerUpdates() override;

	private:
		void Slot_Keypad_Ack(const Messages::JandyMessage_Ack& msg);
		void Slot_Keypad_Probe(const Messages::JandyMessage_Probe& msg);
		void Slot_Keypad_Message(const Messages::JandyMessage_Message& msg);
		void Slot_Keypad_MessageLong(const Messages::JandyMessage_MessageLong& msg);
		void Slot_Keypad_Status(const Messages::JandyMessage_Status& msg);
	};

}
// namespace AqualinkAutomate::Devices
