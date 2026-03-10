#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <string>

#include "devices/jandy_controller.h"
#include "devices/jandy_device_types.h"
#include "devices/capabilities/describable.h"
#include "devices/capabilities/emulated.h"
#include "devices/capabilities/restartable.h"
#include "devices/capabilities/screen.h"
#include "messages/iaq/iaq_message_aux_status.h"
#include "messages/iaq/iaq_message_command_ready.h"
#include "messages/iaq/iaq_message_control_ready.h"
#include "messages/iaq/iaq_message_heartbeat.h"
#include "messages/iaq/iaq_message_main_status.h"
#include "messages/iaq/iaq_message_message_long.h"
#include "messages/iaq/iaq_message_onetouch_status.h"
#include "messages/iaq/iaq_message_page_button.h"
#include "messages/iaq/iaq_message_page_continue.h"
#include "messages/iaq/iaq_message_page_end.h"
#include "messages/iaq/iaq_message_page_message.h"
#include "messages/iaq/iaq_message_page_start.h"
#include "messages/iaq/iaq_message_poll.h"
#include "messages/iaq/iaq_message_startup.h"
#include "messages/iaq/iaq_message_table_message.h"
#include "messages/iaq/iaq_message_title_message.h"
#include "utility/screen_data_page.h"
#include "utility/screen_data_page_updater.h"
#include "kernel/hub_locator.h"
#include "profiling/profiling.h"

namespace AqualinkAutomate::Devices
{

	class IAQDevice : public JandyController, public Capabilities::Restartable, public Capabilities::Screen, public Capabilities::Emulated, public Capabilities::Describable
	{
		inline static const uint8_t IAQ_STATUS_PAGE_LINES = 18;
		inline static const uint8_t IAQ_MESSAGE_TABLE_LINES = 18;
		inline static const std::chrono::seconds IAQ_TIMEOUT_DURATION{ std::chrono::seconds(30) };

		enum class OperatingStates
		{
			StartUp,
			NormalOperation,
			FaultHasOccurred
		};

	public:
		IAQDevice(const std::shared_ptr<Devices::JandyDeviceType>& device_id, Kernel::HubLocator& hub_locator, bool is_emulated);
		~IAQDevice() override;

	public:
		nlohmann::json DescribeDiagnostics() const override;

	public:
		void QueueCommand(uint8_t command);
		void QueueChlorinatorPercentage(uint8_t percentage);
		void QueueChlorinatorBoost(bool enable);

	private:
		void ProcessControllerUpdates() override;
		void ProcessControllerUpdates(bool is_poll_message);

	private:
		void WatchdogTimeoutOccurred() override;

	private:
		void Slot_IAQ_AuxStatus(const Messages::IAQMessage_AuxStatus& msg);
		void Slot_IAQ_CommandReady(const Messages::IAQMessage_CommandReady& msg);
		void Slot_IAQ_ControlReady(const Messages::IAQMessage_ControlReady& msg);
		void Slot_IAQ_Heartbeat(const Messages::IAQMessage_Heartbeat& msg);
		void Slot_IAQ_MainStatus(const Messages::IAQMessage_MainStatus& msg);
		void Slot_IAQ_MessageLong(const Messages::IAQMessage_MessageLong& msg);
		void Slot_IAQ_OneTouchStatus(const Messages::IAQMessage_OneTouchStatus& msg);
		void Slot_IAQ_PageButton(const Messages::IAQMessage_PageButton& msg);
		void Slot_IAQ_PageContinue(const Messages::IAQMessage_PageContinue& msg);
		void Slot_IAQ_PageEnd(const Messages::IAQMessage_PageEnd& msg);
		void Slot_IAQ_PageMessage(const Messages::IAQMessage_PageMessage& msg);
		void Slot_IAQ_PageStart(const Messages::IAQMessage_PageStart& msg);
		void Slot_IAQ_Poll(const Messages::IAQMessage_Poll& msg);
		void Slot_IAQ_StartUp(const Messages::IAQMessage_StartUp& msg);
		void Slot_IAQ_TableMessage(const Messages::IAQMessage_TableMessage& msg);
		void Slot_IAQ_TitleMessage(const Messages::IAQMessage_TitleMessage& msg);

	private:
		void ProcessMainStatus(const Messages::IAQMessage_MainStatus& msg);
		void ProcessAuxStatus(const Messages::IAQMessage_AuxStatus& msg);

	private:
		Utility::ScreenDataPage m_StatusPage;
		Utility::ScreenDataPage m_TableInfo;

	private:
		Utility::ScreenDataPageUpdater<Utility::ScreenDataPage> m_SM_PageUpdate;
		Utility::ScreenDataPageUpdater<Utility::ScreenDataPage> m_SM_TableUpdate;

	private:
		void Signal_ControlDataResponse(const std::string& ascii_data);

	private:
		OperatingStates m_OpState{ OperatingStates::StartUp };
		uint8_t m_PendingCommand{ 0x00 };
		std::deque<uint8_t> m_CommandQueue;
		bool m_AwaitingControlReady{ false };
		std::string m_ControlDataValue;

	private:
		Types::ProfilingUnitTypePtr m_ProfilingDomain;
	};

}
// namespace AqualinkAutomate::Devices
