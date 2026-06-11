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
#include "devices/iaq/iaq_page_registry.h"
#include "messages/jandy_message_probe.h"
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
			FaultHasOccurred,
			NotPresent          // an id the master never addresses (e.g. an emulated iAqualink2 the panel isn't configured for)
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

		// Press an on-screen PageButton on the AqualinkTouch (0x33) page UI by its index
		// (the index carried in the master's IAQMessage_PageButton frames). This lets the
		// emulated panel navigate the master's pages -- open sub-pages, toggle equipment --
		// exactly as a physical touch would.
		void SelectPageButton(uint8_t button_index);

		// Arm a start-up PAGE SURVEY: once the home page is established (first MainStatus), an
		// emulated panel walks `registry`'s data pages -- navigating to each, dwelling so it
		// renders+decodes, then back -- to source data the pushed home page does not carry
		// (setpoints, etc.). Targeted navigation instead of menu spidering. Runs once.
		void EnablePageSurvey(const IAQ::PageRegistry& registry);

	public:
		// Operating-state queries (also exercised by the device tests).
		bool IsInNormalOperation() const { return m_OpState == OperatingStates::NormalOperation; }
		bool IsFaulted() const { return m_OpState == OperatingStates::FaultHasOccurred; }
		bool IsNotPresent() const { return m_OpState == OperatingStates::NotPresent; }

	protected:
		void ProcessControllerUpdates() override;
		void ProcessControllerUpdates(bool is_poll_message);

	protected:
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
		void Slot_IAQ_Probe(const Messages::JandyMessage_Probe& msg);
		void Slot_IAQ_StartUp(const Messages::IAQMessage_StartUp& msg);
		void Slot_IAQ_TableMessage(const Messages::IAQMessage_TableMessage& msg);
		void Slot_IAQ_TitleMessage(const Messages::IAQMessage_TitleMessage& msg);

	private:
		void ProcessMainStatus(const Messages::IAQMessage_MainStatus& msg);
		void ProcessAuxStatus(const Messages::IAQMessage_AuxStatus& msg);

		// Once home is established, queue the page-survey navigation sequence (built from the
		// registry) so it drains one command per poll. Emulated + survey-enabled + not-run only.
		void MaybeStartPageSurvey();

	private:
		// Render the live decoded system status into the device's Screen capability
		// as a fixed "System Status" page so the diagnostics "Actual Devices" card
		// shows real data instead of Page_Unknown.  The IAQ (iAqualink2 cloud
		// interface) has no navigable physical screen, so its screen is a reflection
		// of the status it just decoded from MainStatus (+ DataHub aux state).
		void RenderStatusScreen(const Messages::IAQMessage_MainStatus& msg);

		// Render a fixed "Cloud Link" page for a heartbeat-only IAQ (the iAqualink2
		// cloud interface on 0xA3) which receives ONLY the heartbeat (0x53) and never
		// a MainStatus/AuxStatus or any navigable page.  Without this it would sit on
		// the constructor-default Page_Unknown forever.  Mirrors RenderStatusScreen
		// but the content is the heartbeat liveness, not decoded system status.
		void RenderCloudLinkScreen();

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
		bool m_HasReceivedData{ false };       // has any traffic ever been addressed to this id? (distinguishes "not present" from "went silent")
		bool m_HasReceivedMainStatus{ false }; // has a MainStatus ever decoded? (a 0x33 renders System Status; a heartbeat-only 0xA3 renders Cloud Link)
		uint8_t m_PendingCommand{ 0x00 };
		std::deque<uint8_t> m_CommandQueue;
		bool m_AwaitingControlReady{ false };
		std::string m_ControlDataValue;

		bool m_PageSurveyEnabled{ false };       // arm a start-up data-page survey (auto-startup page-push)
		bool m_PageSurveyDone{ false };          // the survey runs once, after home is established
		IAQ::PageRegistry m_PageSurveyRegistry;  // the declarative pages to visit

	private:
		Types::ProfilingUnitTypePtr m_ProfilingDomain;
	};

}
// namespace AqualinkAutomate::Devices
