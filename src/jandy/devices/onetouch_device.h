#pragma once

#include <array>
#include <chrono>
#include <list>
#include <memory>
#include <string_view>
#include <vector>

#include "devices/jandy_controller.h"
#include "devices/jandy_device_types.h"
#include "devices/capabilities/emulated.h"
#include "devices/capabilities/restartable.h"
#include "devices/capabilities/screen.h"
#include "messages/jandy_message_ack.h"
#include "messages/jandy_message_ids.h"
#include "messages/jandy_message_probe.h"
#include "messages/jandy_message_message_long.h"
#include "messages/jandy_message_status.h"
#include "messages/jandy_message_unknown.h"
#include "messages/pda/pda_message_clear.h"
#include "messages/pda/pda_message_highlight.h"
#include "messages/pda/pda_message_highlight_chars.h"
#include "messages/pda/pda_message_shiftlines.h"
#include "navigation/menu_model.h"
#include "navigation/navigator.h"
#include "navigation/scrape_task.h"
#include "kernel/hub_locator.h"
#include "profiling/profiling.h"

namespace AqualinkAutomate::Devices
{

	class OneTouchDevice : public JandyController, public Capabilities::Restartable, public Capabilities::Screen, public Capabilities::Emulated
	{
		inline static const uint8_t ONETOUCH_PAGE_LINES = 12;
		inline static const std::chrono::seconds ONETOUCH_TIMEOUT_DURATION{ std::chrono::seconds(30) };
		inline static const uint32_t ONETOUCH_SCRAPING_STALL_LIMIT{ 10 };

		enum class OperatingStates
		{
			StartUp,
			Scraping,           // Navigator and tasks are running
			NormalOperation,
			ScrapingFaulted,    // Scraping failed unrecoverably, device state unknown
			FaultHasOccurred
		};

	public:
		enum class KeyCommands : uint8_t
		{
			NoKeyCommand = 0x00,
			PageDown_Or_Select1 = 0x01,
			Back_Or_Select2 = 0x02,
			PageUp_Or_Select3 = 0x03,
			Select = 0x04,
			LineDown = 0x05,
			LineUp = 0x06,
			Unknown = 0xFF
		};

	public:
		OneTouchDevice(std::shared_ptr<Devices::JandyDeviceType> device_id, Kernel::HubLocator& hub_locator, bool is_emulated);
		virtual ~OneTouchDevice();

	private:
		virtual void ProcessControllerUpdates() override;
		void ProcessControllerUpdates(bool is_status_message);

	private:
		virtual void WatchdogTimeoutOccurred() override;

	private:
		void Slot_OneTouch_Ack(const Messages::JandyMessage_Ack& msg);
		void Slot_OneTouch_MessageLong(const Messages::JandyMessage_MessageLong& msg);
		void Slot_OneTouch_Probe(const Messages::JandyMessage_Probe& msg);
		void Slot_OneTouch_Status(const Messages::JandyMessage_Status& msg);
		void Slot_OneTouch_Clear(const Messages::PDAMessage_Clear& msg);
		void Slot_OneTouch_Highlight(const Messages::PDAMessage_Highlight& msg);
		void Slot_OneTouch_HighlightChars(const Messages::PDAMessage_HighlightChars& msg);
		void Slot_OneTouch_ShiftLines(const Messages::PDAMessage_ShiftLines& msg);
		void Slot_OneTouch_Unknown(const Messages::JandyMessage_Unknown& msg);

	private:
		void PageProcessor_System(const Utility::ScreenDataPage& page);
		void PageProcessor_Service(const Utility::ScreenDataPage& page);
		void PageProcessor_TimeOut(const Utility::ScreenDataPage& page);
		void PageProcessor_OneTouch(const Utility::ScreenDataPage& page);
		void PageProcessor_EquipmentOnOff(const Utility::ScreenDataPage& page);
		void PageProcessor_EquipmentStatus(const Utility::ScreenDataPage& page);
		void PageProcessor_SelectSpeed(const Utility::ScreenDataPage& page);
		void PageProcessor_MenuHelp(const Utility::ScreenDataPage& page);
		void PageProcessor_HelpSubmenu(const Utility::ScreenDataPage& page);
		void PageProcessor_SetTemperature(const Utility::ScreenDataPage& page);
		void PageProcessor_SetTime(const Utility::ScreenDataPage& page);
		void PageProcessor_SystemSetup(const Utility::ScreenDataPage& page);
		void PageProcessor_FreezeProtect(const Utility::ScreenDataPage& page);
		void PageProcessor_Boost(const Utility::ScreenDataPage& page);
		void PageProcessor_SetAquapure(const Utility::ScreenDataPage& page);
		void PageProcessor_Version(const Utility::ScreenDataPage& page);
		void PageProcessor_DiagnosticsSensors(const Utility::ScreenDataPage& page);
		void PageProcessor_DiagnosticsRemotes(const Utility::ScreenDataPage& page);
		void PageProcessor_DiagnosticsErrors(const Utility::ScreenDataPage& page);
		void PageProcessor_LabelAuxList(const Utility::ScreenDataPage& page);
		void PageProcessor_LabelAux(const Utility::ScreenDataPage& page);

	private:
		static const uint32_t HINT_COUNT{ 2 };
		using HintArrayType = std::array<unsigned char, HINT_COUNT>;

		bool StatusProcessor_ShouldSkipLineProcessing(const HintArrayType& hint_array, const std::string_view line_to_process) const;
		void StatusProcessor_FilterPump(const Utility::ScreenDataPage& page, const uint8_t line_id);
		void StatusProcessor_PoolHeat(const Utility::ScreenDataPage& page, const uint8_t line_id);
		void StatusProcessor_SpaHeat(const Utility::ScreenDataPage& page, const uint8_t line_id);
		void StatusProcessor_SolarHeat(const Utility::ScreenDataPage& page, const uint8_t line_id);
		void StatusProcessor_HeatPump(const Utility::ScreenDataPage& page, const uint8_t line_id);
		void StatusProcessor_Chiller(const Utility::ScreenDataPage& page, const uint8_t line_id);
		void StatusProcessor_AquaPurePercentage(const Utility::ScreenDataPage& page, const uint8_t line_id);
		void StatusProcessor_SaltLevelPPM(const Utility::ScreenDataPage& page, const uint8_t line_id);
		void StatusProcessor_CheckAquaPure(const Utility::ScreenDataPage& page, const uint8_t line_id);

	private:
		// Navigation-based scraping
		void Scraping_ProcessStep_StartUp();
		void Scraping_ProcessStep();

		// Convert Navigator key command to device KeyCommand
		static KeyCommands ConvertNavKeyCommand(Navigation::NavKeyCommand nav_cmd);

		// Callbacks for scraping tasks
		void OnAuxLabelScraped(uint8_t aux_index, const std::string& label);
		void OnEquipmentStatusScraped(const Utility::ScreenDataPage& status_page);
		void OnDiagnosticsScraped(
			const Utility::ScreenDataPage& sensors_page,
			const Utility::ScreenDataPage& remotes_page,
			const Utility::ScreenDataPage& errors_page);

	private:
		// Navigation system
		Navigation::MenuModel m_MenuModel;
		std::unique_ptr<Navigation::Navigator> m_Navigator;
		std::unique_ptr<Navigation::ScrapeTask> m_CurrentTask;

	private:
		OperatingStates m_OpState{ OperatingStates::StartUp };
		uint32_t m_ScrapingStallCounter{ 0 };
		uint8_t m_HighlightedLine{ 0 };

	private:
		// AqualinkD always uses 0x80 (V2_Normal) for ACKs. The controller may ignore
	// 0x00 responses and fail to register the device, so start with V2_Normal.
	Messages::AckTypes m_AckType_ToSend{ Messages::AckTypes::V2_Normal };
		KeyCommands m_KeyCommand_ToSend{ KeyCommands::NoKeyCommand };

	private:
		Types::ProfilingUnitTypePtr m_ProfilingDomain;
	};

}
// namespace AqualinkAutomate::Devices
