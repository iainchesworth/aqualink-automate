#pragma once

#include <chrono>

#include "interfaces/idevice.h"
#include "jandy/messages/jandy_message_message_long.h"
#include "jandy/messages/pda/pda_message_clear.h"
#include "jandy/messages/pda/pda_message_highlight.h"
#include "jandy/messages/pda/pda_message_highlight_chars.h"
#include "jandy/messages/pda/pda_message_shiftlines.h"
#include "jandy/utility/screen_data_page.h"
#include "jandy/utility/screen_data_page_updater.h"

namespace AqualinkAutomate::Devices
{

	class OneTouchDevice : public Interfaces::IDevice
	{
		static const uint8_t ONETOUCH_PAGE_LINES = 12;

		const std::chrono::seconds ONETOUCH_TIMEOUT_DURATION = std::chrono::seconds(30);

	public:
		OneTouchDevice(boost::asio::io_context& io_context, IDevice::DeviceId id);
		virtual ~OneTouchDevice();

	private:
		void Slot_OneTouch_MessageLong(const Messages::JandyMessage_MessageLong& msg);
		void Slot_OneTouch_Clear(const Messages::PDAMessage_Clear& msg);
		void Slot_OneTouch_Highlight(const Messages::PDAMessage_Highlight& msg);
		void Slot_OneTouch_HighlightChars(const Messages::PDAMessage_HighlightChars& msg);
		void Slot_OneTouch_ShiftLines(const Messages::PDAMessage_ShiftLines& msg);

	private:
		Utility::ScreenDataPage<ONETOUCH_PAGE_LINES> m_DisplayedPage;
		Utility::ScreenDataPageUpdater<Utility::ScreenDataPage<ONETOUCH_PAGE_LINES>> m_DisplayedPageUpdater;
	};

}
// namespace AqualinkAutomate::Devices