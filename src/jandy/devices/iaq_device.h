#pragma once

#include <chrono>

#include <boost/asio/io_context.hpp>

#include "interfaces/idevice.h"
#include "jandy/messages/iaq/iaq_message_control_ready.h"
#include "jandy/messages/iaq/iaq_message_message_long.h"
#include "jandy/messages/iaq/iaq_message_page_button.h"
#include "jandy/messages/iaq/iaq_message_page_continue.h"
#include "jandy/messages/iaq/iaq_message_page_end.h"
#include "jandy/messages/iaq/iaq_message_page_message.h"
#include "jandy/messages/iaq/iaq_message_page_start.h"
#include "jandy/messages/iaq/iaq_message_poll.h"
#include "jandy/messages/iaq/iaq_message_startup.h"
#include "jandy/messages/iaq/iaq_message_table_message.h"
#include "jandy/utility/screen_data_page.h"
#include "jandy/utility/screen_data_page_updater.h"

namespace AqualinkAutomate::Devices
{

	class IAQDevice : public Interfaces::IDevice
	{
		static const uint8_t IAQ_STATUS_PAGE_LINES = 18;
		static const uint8_t IAQ_MESSAGE_TABLE_LINES = 18;

		const std::chrono::seconds IAQ_TIMEOUT_DURATION = std::chrono::seconds(30);

	public:
		IAQDevice(boost::asio::io_context& io_context, IDevice::DeviceId id);
		virtual ~IAQDevice();

	private:
		void Slot_IAQ_ControlReady(const Messages::IAQMessage_ControlReady& msg);
		void Slot_IAQ_MessageLong(const Messages::IAQMessage_MessageLong& msg);
		void Slot_IAQ_PageButton(const Messages::IAQMessage_PageButton& msg);
		void Slot_IAQ_PageContinue(const Messages::IAQMessage_PageContinue& msg);
		void Slot_IAQ_PageEnd(const Messages::IAQMessage_PageEnd& msg);
		void Slot_IAQ_PageMessage(const Messages::IAQMessage_PageMessage& msg);
		void Slot_IAQ_PageStart(const Messages::IAQMessage_PageStart& msg);
		void Slot_IAQ_Poll(const Messages::IAQMessage_Poll& msg);
		void Slot_IAQ_StartUp(const Messages::IAQMessage_StartUp& msg);
		void Slot_IAQ_TableMessage(const Messages::IAQMessage_TableMessage& msg);

	private:
		Utility::ScreenDataPage m_StatusPage;
		Utility::ScreenDataPage m_TableInfo;

	private:
		Utility::ScreenDataPageUpdater<Utility::ScreenDataPage> m_SM_PageUpdate;
		Utility::ScreenDataPageUpdater<Utility::ScreenDataPage> m_SM_TableUpdate;
	};

}
// namespace AqualinkAutomate::Devices
