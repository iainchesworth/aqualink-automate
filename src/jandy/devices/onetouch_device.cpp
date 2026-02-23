#include <functional>

#include <magic_enum/magic_enum.hpp>

#include "logging/logging.h"
#include "devices/device_status.h"
#include "devices/onetouch_device.h"
#include "formatters/jandy_device_formatters.h"
#include "kernel/body_of_water.h"
#include "kernel/body_of_water_ids.h"
#include "navigation/onetouch_menu_model.h"
#include "navigation/visit_policies.h"
#include "utility/jandy_pool_configuration_decoder.h"
#include "utility/screen_data_page_processor.h"
#include "utility/string_manipulation.h"

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
		m_SpiderEngine(std::make_unique<Navigation::SpiderEngine>(m_MenuModel, *m_Navigator)),
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
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_OneTouch, { 11, "System" }, std::bind(&OneTouchDevice::PageProcessor_OneTouch, this, std::placeholders::_1)),
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
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_LabelAux, { 2, "Current Label" }, std::bind(&OneTouchDevice::PageProcessor_LabelAux, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetPoolHeat, { 0, "Pool Heat" }, std::bind(&OneTouchDevice::PageProcessor_SetPoolHeat, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetSpaHeat, { 0, "Spa Heat" }, std::bind(&OneTouchDevice::PageProcessor_SetSpaHeat, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_MoreOneTouch, { 10, "OneTouch ON/OFF" }, std::bind(&OneTouchDevice::PageProcessor_MoreOneTouch, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Program, { 0, "Program" }, std::bind(&OneTouchDevice::PageProcessor_Program, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_DisplayLight, { 0, "Display Light" }, std::bind(&OneTouchDevice::PageProcessor_DisplayLight, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Lockouts, { 0, "Lockout" }, std::bind(&OneTouchDevice::PageProcessor_Lockouts, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_PasswordSettings, { 0, "Password" }, std::bind(&OneTouchDevice::PageProcessor_PasswordSettings, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_ProgramGroup, { 0, "Program Group" }, std::bind(&OneTouchDevice::PageProcessor_ProgramGroup, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_GeneralLabels, { 0, "General" }, std::bind(&OneTouchDevice::PageProcessor_GeneralLabels, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_LightLabels, { 0, "Light" }, std::bind(&OneTouchDevice::PageProcessor_LightLabels, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_WaterfallLabels, { 0, "Wtrfall" }, std::bind(&OneTouchDevice::PageProcessor_WaterfallLabels, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_CustomLabel, { 0, "Custom" }, std::bind(&OneTouchDevice::PageProcessor_CustomLabel, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_EnterPassword, { 0, "Enter Password" }, std::bind(&OneTouchDevice::PageProcessor_EnterPassword, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_HelpKeys, { 0, "Key Help" }, std::bind(&OneTouchDevice::PageProcessor_HelpKeys, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_StartUp, { 5, "-" }, std::bind(&OneTouchDevice::PageProcessor_StartUp, this, std::placeholders::_1))
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
		m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_DisplayUpdate>(std::bind(&OneTouchDevice::Slot_OneTouch_DisplayUpdate, this, std::placeholders::_1), (*device_id)());
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
		if (!IsEmulated() && (m_OpState == OperatingStates::ColdStart || m_OpState == OperatingStates::StartUp))
		{
			LogInfo(Channel::Devices, std::format("OneTouch ({}): Non-emulated device detected - skipping scraping, entering NormalOperation", DeviceId()));
			m_OpState = OperatingStates::NormalOperation;
			Status(Devices::DeviceStatus_Normal{});
		}

		switch (m_OpState)
		{
		case OperatingStates::ColdStart:
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::ProcessControllerUpdates -> cold_start", std::source_location::current());
			auto detected = m_MenuModel.DetectPage(DisplayedPage());
			if (detected == Navigation::PageId::StartUp)
			{
				// Splash is still showing - stay in ColdStart and wait.
				// Page processor has already extracted model/type/revision.
				LogDebug(Channel::Devices, std::format("OneTouch ({}): Cold start splash active - waiting for controller to transition", DeviceId()));
			}
			else if (detected != Navigation::PageId::Unknown)
			{
				// Controller has moved past splash to a real page - start spider
				LogInfo(Channel::Devices, std::format("OneTouch ({}): Controller ready - proceeding to StartUp", DeviceId()));
				m_OpState = OperatingStates::StartUp;
				Scraping_ProcessStep_StartUp();
			}
			// else: Unknown page - stay in ColdStart, waiting for recognisable screen
			break;
		}

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
		else if (m_OpState == OperatingStates::StartUp || m_OpState == OperatingStates::ColdStart)
		{
			// Never received a recognisable page during start-up.
			LogWarning(Channel::Devices, std::format("OneTouch({}) : No valid page received during startup -> entering FaultHasOccurred", DeviceId()));
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

	void OneTouchDevice::Scraping_ProcessStep_StartUp()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::Scraping_ProcessStep_StartUp", std::source_location::current());

		LogDebug(Channel::Devices, std::format("OneTouch ({}): Processing StartUp scraping step", DeviceId()));

		Status(Devices::DeviceStatus_Initializing{});

		// Start the SpiderEngine crawl if not already active
		if (m_SpiderEngine->GetState() == Navigation::SpiderEngine::State::Idle)
		{
			// Use FullDiscoveryVisitPolicy for startup - visits all navigable pages
			auto policy = std::make_unique<Navigation::FullDiscoveryVisitPolicy>(
				[this](Navigation::PageId page, const Utility::ScreenDataPage& content)
				{
					LogDebug(Channel::Scraping, std::format("OneTouch ({}): SpiderEngine visited page {}",
						DeviceId(), static_cast<uint32_t>(page)));
				},
				[this]()
				{
					LogInfo(Channel::Scraping, std::format("OneTouch ({}): SpiderEngine startup crawl complete", DeviceId()));
				}
			);
			m_SpiderEngine->StartCrawl(std::move(policy));
		}

		// Transition to Scraping state - the SpiderEngine handles sync internally
		m_OpState = OperatingStates::Scraping;
		m_ScrapingStallCounter = 0;

		// Process the first step immediately
		Scraping_ProcessStep();
	}

	void OneTouchDevice::Scraping_ProcessStep()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::Scraping_ProcessStep", std::source_location::current());

		if (!m_SpiderEngine || !m_Navigator)
		{
			LogWarning(Channel::Scraping, std::format("OneTouch ({}): No active spider engine or navigator", DeviceId()));
			m_OpState = OperatingStates::NormalOperation;
			Status(Devices::DeviceStatus_Normal{});
			return;
		}

		// Delegate to SpiderEngine
		auto nav_cmd = m_SpiderEngine->ProcessStep(DisplayedPage(), m_HighlightedLine);

		// Check engine state
		if (m_SpiderEngine->GetState() == Navigation::SpiderEngine::State::Complete)
		{
			LogInfo(Channel::Scraping, std::format("OneTouch ({}): Startup scrape complete ({} pages visited) - entering NormalOperation",
				DeviceId(), m_SpiderEngine->GetVisitedPages().size()));
			m_Navigator->Reset();
			m_OpState = OperatingStates::NormalOperation;
			Status(Devices::DeviceStatus_Normal{});
			return;
		}

		if (m_SpiderEngine->GetState() == Navigation::SpiderEngine::State::Failed)
		{
			LogError(Channel::Scraping, std::format("OneTouch ({}): SpiderEngine failed - entering ScrapingFaulted", DeviceId()));
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
			// No command - check if we're on a transient page (no edges = controller auto-transitions)
			auto detected = m_MenuModel.DetectPage(DisplayedPage());
			const auto* page_info = (detected != Navigation::PageId::Unknown)
				? m_MenuModel.GetPage(detected) : nullptr;

			if (page_info && page_info->edges.empty())
			{
				// On a transient page - don't count as stall, controller will auto-transition
				LogDebug(Channel::Scraping, std::format("OneTouch ({}): On transient page '{}' - waiting for controller to transition",
					DeviceId(), page_info->name));
				m_ScrapingStallCounter = 0;
			}
			else if (m_Navigator && (m_Navigator->GetState() == Navigation::Navigator::State::WaitingForPage
				|| m_Navigator->GetState() == Navigation::Navigator::State::MovingCursor))
			{
				// Navigator is actively waiting for a Status message to decrement the
				// pending counter — non-Status messages (MessageLong, Highlight, etc.)
				// trigger ProcessStep but that's not a stall.
				m_ScrapingStallCounter = 0;
			}
			else
			{
				m_ScrapingStallCounter++;

				if (m_ScrapingStallCounter >= ONETOUCH_SCRAPING_STALL_LIMIT)
				{
					LogWarning(Channel::Scraping, std::format("OneTouch ({}): Scraping stalled for {} iterations",
						DeviceId(), m_ScrapingStallCounter));
					// Reset stall counter and let the spider engine handle recovery
					m_ScrapingStallCounter = 0;
				}
			}
		}
	}

	void OneTouchDevice::PageProcessor_HelpSubmenu(const Utility::ScreenDataPage& page)
	{
		LogTrace(Channel::Devices, std::format("OneTouch ({}): PageProcessor_HelpSubmenu invoked", DeviceId()));
	}

	void OneTouchDevice::PageProcessor_StartUp(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::PageProcessor_StartUp", std::source_location::current());

		LogDebug(Channel::Devices, std::format("OneTouch ({}): Processing cold start splash screen", DeviceId()));

		const auto model_number = Utility::TrimWhitespace(page[4].Text);
		const auto panel_type = Utility::TrimWhitespace(page[5].Text);
		const auto fw_revision = Utility::TrimWhitespace(page[7].Text);

		Utility::PoolConfigurationDecoder pool_config_decoder(panel_type);

		// Handle autodetect vs user-specified configuration.
		if (JandyController::m_DataHub->PoolConfigurationSource == Kernel::ConfigurationSource::UserSpecified
			&& pool_config_decoder.Configuration() != JandyController::m_DataHub->PoolConfiguration)
		{
			LogWarning(Channel::Equipment, std::format("Autodetected pool configuration '{}' disagrees with user-specified '{}'",
				magic_enum::enum_name(pool_config_decoder.Configuration()),
				magic_enum::enum_name(JandyController::m_DataHub->PoolConfiguration)));
			// User specification takes precedence; do not override.
		}
		else
		{
			JandyController::m_DataHub->PoolConfiguration = pool_config_decoder.Configuration();
		}

		JandyController::m_DataHub->SystemBoard = pool_config_decoder.SystemBoard();
		JandyController::m_DataHub->EquipmentVersions.Set("Model", model_number);
		JandyController::m_DataHub->EquipmentVersions.Set("Type", panel_type);
		JandyController::m_DataHub->EquipmentVersions.Set("Revision", fw_revision);

		// Populate bodies if not already present (user config may have done this at startup).
		if (JandyController::m_DataHub->Bodies().empty())
		{
			switch (JandyController::m_DataHub->PoolConfiguration)
			{
			case Kernel::PoolConfigurations::DualBody_SharedEquipment:
			case Kernel::PoolConfigurations::DualBody_DualEquipment:
				JandyController::m_DataHub->AddBody(Kernel::BodyOfWater{ Kernel::BodyOfWaterIds::Pool, "Pool" });
				JandyController::m_DataHub->AddBody(Kernel::BodyOfWater{ Kernel::BodyOfWaterIds::Spa, "Spa" });
				break;

			case Kernel::PoolConfigurations::SingleBody:
				JandyController::m_DataHub->AddBody(Kernel::BodyOfWater{ Kernel::BodyOfWaterIds::Pool, "Pool" });
				break;

			default:
				break;
			}
		}

		LogInfo(Channel::Devices, std::format("Aqualink Power Center - Model: {}, Type: {}, Rev: {}", model_number, panel_type, fw_revision));
	}

}
// namespace AqualinkAutomate::Devices
