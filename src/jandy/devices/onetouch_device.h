#pragma once

#include <array>
#include <chrono>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "devices/jandy_controller.h"
#include "devices/jandy_device_types.h"
#include "devices/capabilities/chlorinator_controller.h"
#include "devices/capabilities/describable.h"
#include "devices/capabilities/device_actuator.h"
#include "devices/capabilities/emulated.h"
#include "devices/capabilities/restartable.h"
#include "devices/capabilities/screen.h"
#include "devices/capabilities/setpoint_controller.h"
#include "messages/jandy_message_ack.h"
#include "messages/jandy_message_ids.h"
#include "messages/jandy_message_probe.h"
#include "messages/jandy_message_message_long.h"
#include "messages/jandy_message_status.h"
#include "messages/jandy_message_display_update.h"
#include "messages/jandy_message_unknown.h"
#include "messages/pda/pda_message_clear.h"
#include "messages/pda/pda_message_highlight.h"
#include "messages/pda/pda_message_highlight_chars.h"
#include "messages/pda/pda_message_shiftlines.h"
#include "navigation/menu_model.h"
#include "navigation/navigator.h"
#include "navigation/spider_engine.h"
#include "kernel/hub_locator.h"
#include "profiling/profiling.h"

namespace AqualinkAutomate::Devices
{

	class OneTouchDevice : public JandyController, public Capabilities::Restartable, public Capabilities::Screen, public Capabilities::Emulated, public Capabilities::Describable, public Capabilities::DeviceActuator, public Capabilities::SetpointController, public Capabilities::ChlorinatorController
	{
		inline static const uint8_t ONETOUCH_PAGE_LINES = 12;
		inline static const std::chrono::seconds ONETOUCH_TIMEOUT_DURATION{ std::chrono::seconds(30) };
		inline static const uint32_t ONETOUCH_SCRAPING_STALL_LIMIT{ 10 };
		inline static const uint32_t ONETOUCH_ACTUATION_STEP_LIMIT{ 500 };  // frame backstop so a toggle goal can never wedge NormalOperation (the Navigator's own timeouts normally end it first)
		inline static const uint32_t ONETOUCH_VALUEEDIT_STEP_LIMIT{ 500 };  // frame backstop for a value-edit goal (navigation + the value-step loop)
		inline static const uint32_t ONETOUCH_BOOST_STEP_LIMIT{ 500 };      // frame backstop for a chlorinator-boost goal

		enum class OperatingStates
		{
			ColdStart,          // Waiting for initial splash screen from controller
			StartUp,            // Spider engine initialisation
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
		OneTouchDevice(const std::shared_ptr<Devices::JandyDeviceType>& device_id, Kernel::HubLocator& hub_locator, bool is_emulated);
		~OneTouchDevice() override;

	public:
		nlohmann::json DescribeDiagnostics() const override;

		// True when the DataHub already carries one or more aux devices with a
		// non-empty label (e.g. seeded passively by a real iAqualink2 on the bus).
		// When so, the emulated OneTouch skips the slow "Label Aux" menu crawl at
		// startup. Exposed for diagnostics and for validating the skip decision.
		bool DataHubHasSeededAuxLabels() const;

	public:
		// DeviceActuator: actuate (toggle/on/off) a pool device by driving the emulated
		// keypad to the Equipment ON/OFF page and Selecting the row whose label matches
		// the device. Ranks Low so a Serial Adapter (direct command) is preferred when
		// both controllers are present.
		Capabilities::ActuationResult ActuateDevice(const std::shared_ptr<Kernel::AuxillaryDevice>& device, Capabilities::ActuationAction action) override;
		Capabilities::ActuationPriority ControllerPriority() const override { return Capabilities::ActuationPriority::Low; }

		// SetpointController: change the pool/spa heater setpoint by driving the emulated
		// keypad to the Set Temperature page, Select-ing the Pool Heat / Spa Heat row to
		// enter the in-place editor, arrow-stepping the value, then Select-ing again to
		// commit. The value arrives already in the system's configured units (the setpoints
		// route converts before dispatch; the dispatcher validates the range), so it matches
		// the on-screen value 1:1. (ControllerPriority() above satisfies the DeviceActuator,
		// SetpointController AND ChlorinatorController mixins - identical signature.)
		Capabilities::ActuationResult SetPoolSetpoint(uint8_t temperature) override;
		Capabilities::ActuationResult SetSpaSetpoint(uint8_t temperature) override;

		// ChlorinatorController: set the chlorinator output % via the Set AquaPure page (same
		// value-editor as the setpoint, but 5% steps) and start/stop the 100% boost via the
		// Boost Pool page. The % drives the POOL chlorination row to match the IAQ's single-%
		// behaviour; targets are rounded to a multiple of 5 (the OneTouch's step). Ranks Low,
		// so the IAQ's direct value-submit chlorinator (Medium) is preferred on a combined rig.
		Capabilities::ActuationResult SetChlorinatorPercentage(uint8_t percentage) override;
		Capabilities::ActuationResult SetChlorinatorBoost(bool enable) override;

	private:
		void ProcessControllerUpdates() override;
		void ProcessControllerUpdates(bool is_status_message);

	private:
		void WatchdogTimeoutOccurred() override;

	private:
		void Slot_OneTouch_Ack(const Messages::JandyMessage_Ack& msg);
		void Slot_OneTouch_MessageLong(const Messages::JandyMessage_MessageLong& msg);
		void Slot_OneTouch_Probe(const Messages::JandyMessage_Probe& msg);
		void Slot_OneTouch_Status(const Messages::JandyMessage_Status& msg);
		void Slot_OneTouch_Clear(const Messages::PDAMessage_Clear& msg);
		void Slot_OneTouch_Highlight(const Messages::PDAMessage_Highlight& msg);
		void Slot_OneTouch_HighlightChars(const Messages::PDAMessage_HighlightChars& msg);
		void Slot_OneTouch_ShiftLines(const Messages::PDAMessage_ShiftLines& msg);
		void Slot_OneTouch_DisplayUpdate(const Messages::JandyMessage_DisplayUpdate& msg);
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
		void PageProcessor_MoreOneTouch(const Utility::ScreenDataPage& page);
		void PageProcessor_SetPoolHeat(const Utility::ScreenDataPage& page);
		void PageProcessor_SetSpaHeat(const Utility::ScreenDataPage& page);
		void PageProcessor_Program(const Utility::ScreenDataPage& page);
		void PageProcessor_DisplayLight(const Utility::ScreenDataPage& page);
		void PageProcessor_Lockouts(const Utility::ScreenDataPage& page);
		void PageProcessor_PasswordSettings(const Utility::ScreenDataPage& page);
		void PageProcessor_ProgramGroup(const Utility::ScreenDataPage& page);
		void PageProcessor_GeneralLabels(const Utility::ScreenDataPage& page);
		void PageProcessor_LightLabels(const Utility::ScreenDataPage& page);
		void PageProcessor_WaterfallLabels(const Utility::ScreenDataPage& page);
		void PageProcessor_CustomLabel(const Utility::ScreenDataPage& page);
		void PageProcessor_EnterPassword(const Utility::ScreenDataPage& page);
		void PageProcessor_HelpKeys(const Utility::ScreenDataPage& page);
		void PageProcessor_StartUp(const Utility::ScreenDataPage& page);

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

		// On-demand actuation (DeviceActuator): service a single pending toggle goal in
		// NormalOperation by driving the Navigator, and read a device's current on/off
		// state so an explicit On/Off only acts when the state actually differs.
		void Actuation_ProcessStep();
		std::optional<bool> CurrentOnState(const std::shared_ptr<Kernel::AuxillaryDevice>& device) const;

		// On-demand on-screen VALUE EDITOR (SetpointController + chlorinator %): service a
		// single pending value-edit goal in NormalOperation. The Navigator positions the
		// cursor on the goal's value row (no Select); then this device drives the value-step
		// loop directly - Select to begin editing, arrow keys to step the displayed value
		// toward the target, Select to commit. Used for heater setpoints (Set Temperature,
		// 1-degree steps) and chlorinator % (Set AquaPure, 5% steps) alike.
		void ValueEdit_ProcessStep();

		// On-demand chlorinator BOOST (ChlorinatorController): service a single pending
		// boost start/stop goal in NormalOperation via the Boost Pool page - Select on the
		// "Operate at 100%" page starts it; navigating to the "Stop" item and Select stops it.
		void Boost_ProcessStep();

		// Read the currently displayed integer value from a row (e.g. "Pool Heat   90`F" -> 90,
		// "Set Pool to: 45%" -> 45). Returns nullopt when no value is parseable yet (page not
		// rendered, or value blanked mid-edit), so the step loop simply waits.
		std::optional<int> DisplayedValue(uint8_t line_id) const;

		// True when any on-demand goal (toggle / value-edit / boost) is mid-flight; the single
		// shared Navigator means goals must never interleave.
		bool GoalInProgress() const;

		// Convert Navigator key command to device KeyCommand
		static KeyCommands ConvertNavKeyCommand(Navigation::NavKeyCommand nav_cmd);

	private:
		// Navigation system
		Navigation::MenuModel m_MenuModel;
		std::unique_ptr<Navigation::Navigator> m_Navigator;
		std::unique_ptr<Navigation::SpiderEngine> m_SpiderEngine;

	private:
		OperatingStates m_OpState{ OperatingStates::ColdStart };
		uint32_t m_ScrapingStallCounter{ 0 };
		uint8_t m_HighlightedLine{ 0 };

	private:
		// On-demand equipment actuation goal (a single toggle at a time). Set by
		// ActuateDevice (DeviceActuator), serviced by Actuation_ProcessStep in
		// NormalOperation; the Navigator drives the keypad to the row and Selects it.
		std::optional<std::string> m_PendingActuationLabel;
		bool m_ActuationInProgress{ false };
		uint32_t m_ActuationStepCount{ 0 };

	private:
		// On-demand on-screen value-edit goal (a single edit at a time). Set by the
		// SetpointController / ChlorinatorController-% methods, serviced by
		// ValueEdit_ProcessStep in NormalOperation. The phase machine is identical for heater
		// setpoints and chlorinator % - only the page/row/target/units differ.
		enum class ValueEditPhase
		{
			Navigating,     // Navigator positioning the cursor on the value row
			BeginEdit,      // Select pressed to enter the in-place value editor
			Stepping,       // arrow keys stepping the displayed value toward the target
			Commit          // Select pressed again to commit the value and leave the editor
		};

		struct ValueEditGoal
		{
			Navigation::PageId page;   // page hosting the value row (Set Temperature / Set AquaPure)
			uint8_t line;              // row index of the value (cursor target + value read-back)
			std::string label;         // row label for content-based cursor positioning
			uint8_t target;            // target value in the on-screen units
			std::string desc;          // human description for logging
		};

		// Value rows (see PageProcessor_SetTemperature / the Set AquaPure page): on Set
		// Temperature, Pool Heat is line 2 / Spa Heat line 3; on Set AquaPure, "Set Pool to:"
		// is line 3 (verified vs onetouch_chlorinator.cap).
		inline static const uint8_t SETTEMP_POOL_HEAT_LINE{ 2 };
		inline static const uint8_t SETTEMP_SPA_HEAT_LINE{ 3 };
		inline static const uint8_t SETAQUAPURE_POOL_LINE{ 3 };
		inline static const uint8_t ONETOUCH_CHLORINATOR_STEP{ 5 };  // the OneTouch edits AquaPure % in 5% increments

		std::optional<ValueEditGoal> m_PendingValueEdit;
		ValueEditPhase m_ValueEditPhase{ ValueEditPhase::Navigating };
		bool m_ValueEditInProgress{ false };
		uint32_t m_ValueEditStepCount{ 0 };

		// Shared body of the value-edit capability methods: validate the device can actuate,
		// reject if another goal is mid-flight, then enqueue the goal.
		Capabilities::ActuationResult QueueValueEdit(ValueEditGoal goal);

	private:
		// On-demand chlorinator-boost goal (start/stop, one at a time). Set by
		// SetChlorinatorBoost, serviced by Boost_ProcessStep in NormalOperation.
		enum class BoostPhase
		{
			Navigating,     // Navigator driving to the Boost Pool page
			Acting,         // pressing Select (start) or selecting the Stop item (stop)
			Settle          // waiting for the action to take effect / complete
		};
		std::optional<bool> m_PendingBoost;   // true = start, false = stop
		BoostPhase m_BoostPhase{ BoostPhase::Navigating };
		bool m_BoostInProgress{ false };
		uint32_t m_BoostStepCount{ 0 };

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
