#pragma once

#include <chrono>
#include <list>

#include <boost/asio/io_context.hpp>

#include "jandy/devices/jandy_controller.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/devices/capabilities/emulated.h"
#include "jandy/devices/capabilities/scrapeable.h"
#include "jandy/devices/capabilities/screen.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_probe.h"
#include "jandy/messages/jandy_message_message_long.h"
#include "jandy/messages/jandy_message_status.h"
#include "jandy/messages/jandy_message_unknown.h"
#include "jandy/messages/pda/pda_message_clear.h"
#include "jandy/messages/pda/pda_message_highlight.h"
#include "jandy/messages/pda/pda_message_highlight_chars.h"
#include "jandy/messages/pda/pda_message_shiftlines.h"
#include "kernel/data_hub.h"

namespace AqualinkAutomate::Devices
{

	class PDADevice : public JandyController, public Capabilities::Screen, public Capabilities::Scrapeable, public Capabilities::Emulated
	{
		inline static const uint8_t PDA_PAGE_LINES{ 10 };
		inline static const Scrapeable::ScrapeId PDA_CONFIG_INIT_SCRAPER{ 1 };
		inline static const std::chrono::seconds PDA_TIMEOUT_DURATION{ std::chrono::seconds(30) };

		enum class KeyCommands
		{
			NoKeyCommand = 0x00,
			HotKey1 = 0x01,
			Back = 0x02,
			HotKey2 = 0x03,
			Select = 0x04,
			Down = 0x05,
			Up = 0x06,
			Unknown = 0xFF
		};

	public:
		PDADevice(boost::asio::io_context& io_context, std::unique_ptr<Devices::JandyDeviceType>&& device_id, Kernel::DataHub& config, bool is_emulated);
		virtual ~PDADevice();

	private:
		virtual void ProcessControllerUpdates() override;

	private:
		void Slot_PDA_Ack(const Messages::JandyMessage_Ack& msg);
		void Slot_PDA_Clear(const Messages::PDAMessage_Clear& msg);
		void Slot_PDA_Highlight(const Messages::PDAMessage_Highlight& msg);
		void Slot_PDA_HighlightChars(const Messages::PDAMessage_HighlightChars& msg);
		void Slot_PDA_MessageLong(const Messages::JandyMessage_MessageLong& msg);
		void Slot_PDA_Probe(const Messages::JandyMessage_Probe& msg);
		void Slot_PDA_Status(const Messages::JandyMessage_Status& msg);
		void Slot_PDA_ShiftLines(const Messages::PDAMessage_ShiftLines& msg);
		void Slot_PDA_Unknown_PDA_1B(const Messages::JandyMessage_Unknown& msg);

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
