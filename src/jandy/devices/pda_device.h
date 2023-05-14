#pragma once

#include <chrono>

#include <boost/asio/io_context.hpp>

#include "jandy/devices/jandy_device.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_status.h"
#include "jandy/messages/jandy_message_message_long.h"
#include "jandy/messages/jandy_message_unknown.h"
#include "jandy/messages/pda/pda_message_clear.h"
#include "jandy/messages/pda/pda_message_highlight.h"
#include "jandy/messages/pda/pda_message_highlight_chars.h"
#include "jandy/messages/pda/pda_message_shiftlines.h"

namespace AqualinkAutomate::Devices
{

	class PDADevice : public JandyDevice
	{
		const std::chrono::seconds PDA_TIMEOUT_DURATION = std::chrono::seconds(30);

	public:
		PDADevice(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id);
		virtual ~PDADevice();

	private:
		void Slot_PDA_Ack(const Messages::JandyMessage_Ack& msg);
		void Slot_PDA_Clear(const Messages::PDAMessage_Clear& msg);
		void Slot_PDA_Highlight(const Messages::PDAMessage_Highlight& msg);
		void Slot_PDA_HighlightChars(const Messages::PDAMessage_HighlightChars& msg);
		void Slot_PDA_MessageLong(const Messages::JandyMessage_MessageLong& msg);
		void Slot_PDA_Status(const Messages::JandyMessage_Status& msg);
		void Slot_PDA_ShiftLines(const Messages::PDAMessage_ShiftLines& msg);
		void Slot_PDA_Unknown_PDA_1B(const Messages::JandyMessage_Unknown& msg);
	};

}
// namespace AqualinkAutomate::Devices
