#pragma once

#include <chrono>
#include <list>

#include <boost/asio/io_context.hpp>

#include "jandy/devices/jandy_device.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/equipment/jandy_equipment_types.h"
#include "jandy/messages/jandy_message_probe.h"
#include "jandy/messages/jandy_message_message_long.h"
#include "jandy/messages/jandy_message_status.h"
#include "jandy/messages/jandy_message_unknown.h"
#include "jandy/messages/pda/pda_message_clear.h"
#include "jandy/messages/pda/pda_message_highlight.h"
#include "jandy/messages/pda/pda_message_highlight_chars.h"
#include "jandy/messages/pda/pda_message_shiftlines.h"
#include "jandy/utility/screen_data_page.h"
#include "jandy/utility/screen_data_page_processor.h"
#include "jandy/utility/screen_data_page_updater.h"
#include "jandy/utility/temperature.h"

namespace AqualinkAutomate::Devices
{

	class OneTouchDevice : public JandyDevice
	{
		static const uint8_t ONETOUCH_PAGE_LINES = 12;

		const std::chrono::seconds ONETOUCH_TIMEOUT_DURATION = std::chrono::seconds(30);

		enum class OperatingStates
		{
			StartUp,
			InitComplete,
			NormalOperation,
			ScreenUpdating,
			FaultHasOccurred
		};

	public:
		OneTouchDevice(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, JandyDeviceOperatingModes op_mode = JandyDeviceOperatingModes::MonitorOnly);
		virtual ~OneTouchDevice();

	private:
		void HandleAnyInternalProcessing();

	private:
		void Slot_OneTouch_MessageLong(const Messages::JandyMessage_MessageLong& msg);
		void Slot_OneTouch_Probe(const Messages::JandyMessage_Probe& msg);
		void Slot_OneTouch_Status(const Messages::JandyMessage_Status& msg);
		void Slot_OneTouch_Clear(const Messages::PDAMessage_Clear& msg);
		void Slot_OneTouch_Highlight(const Messages::PDAMessage_Highlight& msg);
		void Slot_OneTouch_HighlightChars(const Messages::PDAMessage_HighlightChars& msg);
		void Slot_OneTouch_ShiftLines(const Messages::PDAMessage_ShiftLines& msg);
		void Slot_OneTouch_Unknown(const Messages::JandyMessage_Unknown& msg);

	private:
		void Signal_OneTouch_AckMessage() const;

	private:
		void PageProcessor_Home(const Utility::ScreenDataPage& page);
		void PageProcessor_OneTouch(const Utility::ScreenDataPage& page);
		void PageProcessor_System(const Utility::ScreenDataPage& page);
		void PageProcessor_EquipmentStatus(const Utility::ScreenDataPage& page);
		void PageProcessor_SelectSpeed(const Utility::ScreenDataPage& page);
		void PageProcessor_MenuHelp(const Utility::ScreenDataPage& page);
		void PageProcessor_SetTemperature(const Utility::ScreenDataPage& page);
		void PageProcessor_SetTime(const Utility::ScreenDataPage& page);
		void PageProcessor_SystemSetup(const Utility::ScreenDataPage& page);
		void PageProcessor_FreezeProtect(const Utility::ScreenDataPage& page);
		void PageProcessor_Boost(const Utility::ScreenDataPage& page);
		void PageProcessor_SetAquapure(const Utility::ScreenDataPage& page);
		void PageProcessor_Version(const Utility::ScreenDataPage& page);

	private:
		Utility::ScreenDataPage m_DisplayedPage;
		Utility::ScreenDataPageUpdater<Utility::ScreenDataPage> m_DisplayedPageUpdater;
		std::list<Utility::ScreenDataPage_Processor> m_DisplayedPageProcessors;

	private:
		Equipment::JandyEquipmentTypes m_PanelType{ Equipment::JandyEquipmentTypes::Unknown };
		OperatingStates m_OpState{ OperatingStates::StartUp };
		Utility::Temperature m_AirTemp;
		Utility::Temperature m_PoolTemp;
		Utility::Temperature m_SpaTemp;
		Utility::Temperature m_FreezeProtectPoint;
	};

}
// namespace AqualinkAutomate::Devices
