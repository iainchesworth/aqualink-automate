#pragma once

#include <chrono>
#include <list>

#include <boost/asio/io_context.hpp>

#include "jandy/config/jandy_config.h"
#include "jandy/devices/jandy_controller.h"
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

	class PDADevice : public JandyController
	{
		static const uint8_t PDA_PAGE_LINES = 10;

		const std::chrono::seconds PDA_TIMEOUT_DURATION = std::chrono::seconds(30);

	public:
		PDADevice(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, Config::JandyConfig& config, JandyControllerOperatingModes op_mode);
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

	private:
		void Signal_PDA_AckMessage() const;

	private:
		void PageProcessor_Home(const Utility::ScreenDataPage& page);
		void PageProcessor_SetTemperature(const Utility::ScreenDataPage& page);
		void PageProcessor_SetTime(const Utility::ScreenDataPage& page);
		void PageProcessor_PoolHeat(const Utility::ScreenDataPage& page);
		void PageProcessor_SpaHeat(const Utility::ScreenDataPage& page);
		void PageProcessor_AquaPure(const Utility::ScreenDataPage& page);
		void PageProcessor_AuxLabel(const Utility::ScreenDataPage& page);
		void PageProcessor_FreezeProtect(const Utility::ScreenDataPage& page);
		void PageProcessor_Settings(const Utility::ScreenDataPage& page);
		void PageProcessor_EquipmentStatus(const Utility::ScreenDataPage& page);
		void PageProcessor_Boost(const Utility::ScreenDataPage& page);
		void PageProcessor_FirmwareVersion(const Utility::ScreenDataPage& page);
	};

}
// namespace AqualinkAutomate::Devices
