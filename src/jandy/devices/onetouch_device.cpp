#include <functional>

#include <magic_enum/magic_enum.hpp>

#include "logging/logging.h"
#include "devices/device_status.h"
#include "devices/onetouch_device.h"
#include "formatters/jandy_device_formatters.h"
#include "navigation/onetouch_menu_model.h"
#include "utility/screen_data_page_processor.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Messages;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Devices
{

	OneTouchDevice::OneTouchDevice(const std::shared_ptr<Devices::JandyDeviceType>& device_id, Kernel::HubLocator& hub_locator, bool is_emulated) :
		JandyController(device_id, hub_locator),
		Capabilities::Restartable(ONETOUCH_TIMEOUT_DURATION),
		Capabilities::Screen(ONETOUCH_PAGE_LINES),
		Capabilities::Emulated(is_emulated),
		m_MenuModel(Navigation::CreateOneTouchMenuModel()),
		m_Navigator(std::make_unique<Navigation::Navigator>(m_MenuModel)),
		m_ProfilingDomain(std::move(Factory::ProfilingUnitFactory::Instance().CreateDomain("OneTouchDevice")))
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::OneTouchDevice", std::source_location::current());

		LogInfo(Channel::Devices, std::format("Creating OneTouchDevice: device_id={}, emulated={}, timeout={}s", *device_id, is_emulated, ONETOUCH_TIMEOUT_DURATION.count()));

		m_ProfilingDomain->Start();

		PageProcessors(
			{
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_System, { 9, "Equipment ON/OFF" }, std::bind(&OneTouchDevice::PageProcessor_System, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Service, { 3, "Service Mode" }, std::bind(&OneTouchDevice::PageProcessor_Service, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_TimeOut, { 3, "Timeout Mode" }, std::bind(&OneTouchDevice::PageProcessor_TimeOut, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_OneTouch, { 11, "SYSTEM" }, std::bind(&OneTouchDevice::PageProcessor_OneTouch, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_EquipmentOnOff, { 11, "More" }, std::bind(&OneTouchDevice::PageProcessor_EquipmentOnOff, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_EquipmentOnOff, { 0, "Filter Pump" }, std::bind(&OneTouchDevice::PageProcessor_EquipmentOnOff, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_EquipmentStatus, { 0, "EQUIPMENT STATUS" }, std::bind(&OneTouchDevice::PageProcessor_EquipmentStatus, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SelectSpeed, { 0, "Select Speed" }, std::bind(&OneTouchDevice::PageProcessor_SelectSpeed, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_MenuHelp, { 0, "Menu" }, std::bind(&OneTouchDevice::PageProcessor_MenuHelp, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_HelpSubmenu, { 1, "Keys" }, std::bind(&OneTouchDevice::PageProcessor_HelpSubmenu, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetTemperature, { 0, "Set Temp" }, std::bind(&OneTouchDevice::PageProcessor_SetTemperature, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetTime, { 0, "Set Time" }, std::bind(&OneTouchDevice::PageProcessor_SetTime, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SystemSetup, { 0, "System Setup" }, std::bind(&OneTouchDevice::PageProcessor_SystemSetup, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_FreezeProtect, { 0, "Freeze Protect" }, std::bind(&OneTouchDevice::PageProcessor_FreezeProtect, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Boost, { 0, "Boost Pool" }, std::bind(&OneTouchDevice::PageProcessor_Boost, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetAquapure, { 0, "Set AQUAPURE" }, std::bind(&OneTouchDevice::PageProcessor_SetAquapure, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Version, { 7, "REV " }, std::bind(&OneTouchDevice::PageProcessor_Version, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_DiagnosticsSensors, { 6, "Sensors" }, std::bind(&OneTouchDevice::PageProcessor_DiagnosticsSensors, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_DiagnosticsRemotes, { 0, "Remotes" }, std::bind(&OneTouchDevice::PageProcessor_DiagnosticsRemotes, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_DiagnosticsErrors, { 0, "Errors" }, std::bind(&OneTouchDevice::PageProcessor_DiagnosticsErrors, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_LabelAuxList, { 0, "Label Aux" }, std::bind(&OneTouchDevice::PageProcessor_LabelAuxList, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_LabelAux, { 2, "Current Label" }, std::bind(&OneTouchDevice::PageProcessor_LabelAux, this, std::placeholders::_1))
			}
		);
		LogTrace(Channel::Devices, std::format("OneTouch ({}): Registered {} page processors for OneTouchDevice", DeviceId(), PageProcessors().size()));

		LogDebug(Channel::Devices, std::format("OneTouch ({}): Registering OneTouchDevice message slot handlers", DeviceId()));
		m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_MessageLong>(std::bind(&OneTouchDevice::Slot_OneTouch_MessageLong, this, std::placeholders::_1), (*device_id)());
		m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_Probe>(std::bind(&OneTouchDevice::Slot_OneTouch_Probe, this, std::placeholders::_1), (*device_id)());
		m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_Status>(std::bind(&OneTouchDevice::Slot_OneTouch_Status, this, std::placeholders::_1), (*device_id)());
		m_SlotManager.RegisterSlot_FilterByDeviceId<PDAMessage_Clear>(std::bind(&OneTouchDevice::Slot_OneTouch_Clear, this, std::placeholders::_1), (*device_id)());
		m_SlotManager.RegisterSlot_FilterByDeviceId<PDAMessage_Highlight>(std::bind(&OneTouchDevice::Slot_OneTouch_Highlight, this, std::placeholders::_1), (*device_id)());
		m_SlotManager.RegisterSlot_FilterByDeviceId<PDAMessage_HighlightChars>(std::bind(&OneTouchDevice::Slot_OneTouch_HighlightChars, this, std::placeholders::_1), (*device_id)());
		m_SlotManager.RegisterSlot_FilterByDeviceId<PDAMessage_ShiftLines>(std::bind(&OneTouchDevice::Slot_OneTouch_ShiftLines, this, std::placeholders::_1), (*device_id)());
		m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_Unknown>(std::bind(&OneTouchDevice::Slot_OneTouch_Unknown, this, std::placeholders::_1), (*device_id)());

		if (!IsEmulated())
		{
			LogTrace(Channel::Devices, std::format("OneTouch ({}): Registering ACK handler for non-emulated device", DeviceId()));
			m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_Ack>(std::bind(&OneTouchDevice::Slot_OneTouch_Ack, this, std::placeholders::_1), (*device_id)());
		}

		LogInfo(Channel::Devices, std::format("OneTouch ({}): OneTouchDevice construction complete - device ready", DeviceId()));
	}

	OneTouchDevice::~OneTouchDevice()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::~OneTouchDevice", std::source_location::current());

		LogInfo(Channel::Devices, std::format("OneTouch ({}): Destroying OneTouchDevice: final state was {}", DeviceId(), magic_enum::enum_name(m_OpState)));

		m_ProfilingDomain->End();

		LogTrace(Channel::Devices, std::format("OneTouch ({}): OneTouchDevice destruction complete", DeviceId()));
	}

	void OneTouchDevice::ProcessControllerUpdates()
	{
		// Non-Status message variant - don't send key commands
		ProcessControllerUpdates(false);
	}

	void OneTouchDevice::ProcessControllerUpdates(bool is_status_message)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::ProcessControllerUpdates", std::source_location::current());

		LogTrace(Channel::Devices, std::format("OneTouch ({}): ProcessControllerUpdates called: state={}, is_status={}, pending_cmd={}",
			DeviceId(), magic_enum::enum_name(m_OpState), is_status_message, magic_enum::enum_name(m_KeyCommand_ToSend)));

		// NOTE: Do NOT reset m_KeyCommand_ToSend here. Commands can only be sent in
		// response to Status messages. If a command is set during a non-Status message
		// (e.g., MessageLong/Highlight), it must persist until the next Status message.

		// Non-emulated devices should not run the scraping state machine; they
		// passively observe screens driven by the physical device.  Skip straight
		// to NormalOperation on the first update.
		if (!IsEmulated() && m_OpState == OperatingStates::StartUp)
		{
			LogInfo(Channel::Devices, std::format("OneTouch ({}): Non-emulated device detected - skipping scraping, entering NormalOperation", DeviceId()));
			m_OpState = OperatingStates::NormalOperation;
			Status(Devices::DeviceStatus_Normal{});
		}

		switch (m_OpState)
		{
		case OperatingStates::StartUp:
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::ProcessControllerUpdates -> start_up", std::source_location::current());
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Processing StartUp state", DeviceId()));
			Scraping_ProcessStep_StartUp();
			break;
		}

		case OperatingStates::Scraping:
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::ProcessControllerUpdates -> scraping", std::source_location::current());
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Processing Scraping state", DeviceId()));
			Scraping_ProcessStep();
			break;
		}

		case OperatingStates::NormalOperation:
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::ProcessControllerUpdates -> normal_operation", std::source_location::current());
			LogTrace(Channel::Devices, std::format("OneTouch ({}): Processing NormalOperation state", DeviceId()));
			break;
		}

		case OperatingStates::ScrapingFaulted:
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::ProcessControllerUpdates -> scraping_faulted", std::source_location::current());
			LogWarning(Channel::Scraping, std::format("OneTouch ({}): ScrapingFaulted state - device in unknown state, no commands will be sent", DeviceId()));
			// Do not send any commands - device state is unknown
			break;
		}

		case OperatingStates::FaultHasOccurred:
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::ProcessControllerUpdates -> fault", std::source_location::current());
			LogWarning(Channel::Devices, std::format("OneTouch ({}): Processing FaultHasOccurred state", DeviceId()));
			break;
		}
		}

		// Key commands can ONLY be sent in response to Status messages.
		// The controller ignores key commands in ACKs for other message types.
		if (is_status_message && m_KeyCommand_ToSend != KeyCommands::NoKeyCommand)
		{
			LogTrace(Channel::Devices, std::format("OneTouch({}) : Sending key command in Status ACK: {}", DeviceId(), magic_enum::enum_name(m_KeyCommand_ToSend)));
			// Key commands must be sent with V1_Normal ACK type - the controller ignores
			// key commands in V2_Normal (0x80) ACKs.
			Signal_AckMessage(AckTypes::V1_Normal, m_KeyCommand_ToSend);
			// Clear the command after sending to prevent repeated sends
			m_KeyCommand_ToSend = KeyCommands::NoKeyCommand;
		}
		else
		{
			if (m_KeyCommand_ToSend != KeyCommands::NoKeyCommand)
			{
				LogDebug(Channel::Devices, std::format("OneTouch({}) : Key command {} pending - will send on next Status message", DeviceId(), magic_enum::enum_name(m_KeyCommand_ToSend)));
			}
			Signal_AckMessage(m_AckType_ToSend, KeyCommands::NoKeyCommand);
		}
	}

	void OneTouchDevice::WatchdogTimeoutOccurred()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::WatchdogTimeoutOccurred", std::source_location::current());

		LogWarning(Channel::Devices, std::format("OneTouch({}) : Watchdog timeout occurred: state={}, timeout_duration={}s", DeviceId(), magic_enum::enum_name(m_OpState), ONETOUCH_TIMEOUT_DURATION.count()));

		if (m_OpState == OperatingStates::Scraping)
		{
			// Scraping was in progress when the watchdog fired.  Abandon the
			// scraping sequence and fall through to normal (passive) operation
			// so the device is at least partially functional.
			LogWarning(Channel::Devices, std::format("OneTouch({}) : Abandoning scraping due to watchdog timeout -> entering NormalOperation", DeviceId()));
			m_OpState = OperatingStates::NormalOperation;
			m_ScrapingStallCounter = 0;
			Status(Devices::DeviceStatus_Normal{});
		}
		else if (m_OpState == OperatingStates::StartUp)
		{
			// Never received a recognisable page during start-up.
			LogWarning(Channel::Devices, std::format("OneTouch({}) : No valid page received during StartUp -> entering FaultHasOccurred", DeviceId()));
			m_OpState = OperatingStates::FaultHasOccurred;
			Status(Devices::DeviceStatus_FaultOccurred{});
		}
	}

	OneTouchDevice::KeyCommands OneTouchDevice::ConvertNavKeyCommand(Navigation::NavKeyCommand nav_cmd)
	{
		switch (nav_cmd)
		{
		case Navigation::NavKeyCommand::NoCommand:
			return KeyCommands::NoKeyCommand;
		case Navigation::NavKeyCommand::PageDown_Or_Select1:
			return KeyCommands::PageDown_Or_Select1;
		case Navigation::NavKeyCommand::Back:
			return KeyCommands::Back_Or_Select2;
		case Navigation::NavKeyCommand::PageUp_Or_Select3:
			return KeyCommands::PageUp_Or_Select3;
		case Navigation::NavKeyCommand::Select:
			return KeyCommands::Select;
		case Navigation::NavKeyCommand::LineDown:
			return KeyCommands::LineDown;
		case Navigation::NavKeyCommand::LineUp:
			return KeyCommands::LineUp;
		default:
			return KeyCommands::NoKeyCommand;
		}
	}

	void OneTouchDevice::OnAuxLabelScraped(uint8_t aux_index, const std::string& label)
	{
		LogInfo(Channel::Scraping, std::format("OneTouch ({}): AUX {} label scraped: '{}'",
			DeviceId(), aux_index + 1, label));
		// TODO: Store the label in equipment configuration
	}

	void OneTouchDevice::OnEquipmentStatusScraped(const Utility::ScreenDataPage& status_page)
	{
		LogInfo(Channel::Scraping, std::format("OneTouch ({}): Equipment status scraped", DeviceId()));
		// TODO: Process equipment status
	}

	void OneTouchDevice::OnDiagnosticsScraped(
		const Utility::ScreenDataPage& sensors_page,
		const Utility::ScreenDataPage& remotes_page,
		const Utility::ScreenDataPage& errors_page)
	{
		LogInfo(Channel::Scraping, std::format("OneTouch ({}): Diagnostics scraped (sensors, remotes, errors)", DeviceId()));
		// TODO: Process diagnostics data
	}

	void OneTouchDevice::Scraping_ProcessStep_StartUp()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::Scraping_ProcessStep_StartUp", std::source_location::current());

		LogDebug(Channel::Devices, std::format("OneTouch ({}): Processing StartUp scraping step", DeviceId()));

		Status(Devices::DeviceStatus_Initializing{});

		// Wait for a recognizable starting page
		auto page_type = DisplayedPageType();

		if (page_type == Utility::ScreenDataPageTypes::Page_OneTouch ||
			page_type == Utility::ScreenDataPageTypes::Page_System)
		{
			LogInfo(Channel::Devices, std::format("OneTouch ({}): Detected starting page {} - beginning scrape sequence",
				DeviceId(), magic_enum::enum_name(page_type)));

			// Set the Navigator's current page based on the detected page type
			auto page_id = m_MenuModel.FindPageIdByType(page_type);
			if (page_id != Navigation::PageId::Unknown)
			{
				m_Navigator->SetCurrentPage(page_id);
				LogDebug(Channel::Scraping, std::format("OneTouch ({}): Set navigator current page to {}",
					DeviceId(), static_cast<uint32_t>(page_id)));
			}

			// Create the startup scrape task
			m_CurrentTask = std::make_unique<Navigation::StartupScrapeTask>(
				std::bind(&OneTouchDevice::OnAuxLabelScraped, this, std::placeholders::_1, std::placeholders::_2),
				std::bind(&OneTouchDevice::OnEquipmentStatusScraped, this, std::placeholders::_1),
				std::bind(&OneTouchDevice::OnDiagnosticsScraped, this,
					std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			);

			m_OpState = OperatingStates::Scraping;
			m_ScrapingStallCounter = 0;

			// Process the first step immediately
			Scraping_ProcessStep();
		}
		else
		{
			LogTrace(Channel::Devices, std::format("OneTouch ({}): StartUp waiting for valid page: current_page={}",
				DeviceId(), magic_enum::enum_name(page_type)));
		}
	}

	void OneTouchDevice::Scraping_ProcessStep()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::Scraping_ProcessStep", std::source_location::current());

		if (!m_CurrentTask || !m_Navigator)
		{
			LogWarning(Channel::Scraping, std::format("OneTouch ({}): No active task or navigator", DeviceId()));
			m_OpState = OperatingStates::NormalOperation;
			Status(Devices::DeviceStatus_Normal{});
			return;
		}

		// Keep the Navigator's current page synchronized with page processor detection
		auto page_type = DisplayedPageType();
		if (page_type != Utility::ScreenDataPageTypes::Page_Unknown)
		{
			auto page_id = m_MenuModel.FindPageIdByType(page_type);
			if (page_id != Navigation::PageId::Unknown)
			{
				m_Navigator->SetCurrentPage(page_id);
			}
		}

		// Execute the current task
		auto nav_cmd = m_CurrentTask->Execute(*m_Navigator, DisplayedPage(), m_HighlightedLine);

		// Check task state
		if (m_CurrentTask->GetState() == Navigation::ScrapeTask::State::Completed)
		{
			LogInfo(Channel::Scraping, std::format("OneTouch ({}): Startup scrape complete - entering NormalOperation", DeviceId()));
			m_CurrentTask.reset();
			m_Navigator->Reset();
			m_OpState = OperatingStates::NormalOperation;
			Status(Devices::DeviceStatus_Normal{});
			return;
		}

		if (m_CurrentTask->GetState() == Navigation::ScrapeTask::State::Failed)
		{
			LogError(Channel::Scraping, std::format("OneTouch ({}): Startup scrape failed - entering ScrapingFaulted", DeviceId()));
			m_CurrentTask.reset();
			m_OpState = OperatingStates::ScrapingFaulted;
			Status(Devices::DeviceStatus_FaultOccurred{});
			return;
		}

		// Check navigator state
		if (m_Navigator->GetState() == Navigation::Navigator::State::Failed)
		{
			LogError(Channel::Scraping, std::format("OneTouch ({}): Navigator failed - entering ScrapingFaulted", DeviceId()));
			m_CurrentTask.reset();
			m_OpState = OperatingStates::ScrapingFaulted;
			Status(Devices::DeviceStatus_FaultOccurred{});
			return;
		}

		// If we got a command, convert and queue it
		if (nav_cmd.has_value())
		{
			m_KeyCommand_ToSend = ConvertNavKeyCommand(nav_cmd.value());
			m_ScrapingStallCounter = 0;
			LogDebug(Channel::Scraping, std::format("OneTouch ({}): Sending navigation command: {}",
				DeviceId(), magic_enum::enum_name(m_KeyCommand_ToSend)));
		}
		else
		{
			// No command - we're waiting
			m_ScrapingStallCounter++;

			if (m_ScrapingStallCounter >= ONETOUCH_SCRAPING_STALL_LIMIT)
			{
				LogWarning(Channel::Scraping, std::format("OneTouch ({}): Scraping stalled for {} iterations",
					DeviceId(), m_ScrapingStallCounter));
				// Reset stall counter and let the task/navigator handle recovery
				m_ScrapingStallCounter = 0;
			}
		}
	}

	void OneTouchDevice::PageProcessor_HelpSubmenu(const Utility::ScreenDataPage& page)
	{
		LogTrace(Channel::Devices, std::format("OneTouch ({}): PageProcessor_HelpSubmenu invoked", DeviceId()));
	}

}
// namespace AqualinkAutomate::Devices
