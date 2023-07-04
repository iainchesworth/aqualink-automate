#include <functional>

#include <magic_enum.hpp>

#include "logging/logging.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/utility/screen_data_page_processor.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Messages;

namespace AqualinkAutomate::Devices
{

	OneTouchDevice::OneTouchDevice(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, Kernel::DataHub& config, bool is_emulated) :
		JandyController(io_context, device_id, ONETOUCH_TIMEOUT_DURATION, config),
		Capabilities::Screen(ONETOUCH_PAGE_LINES),
		Capabilities::Scrapeable
		(
			device_id,
			{
				{
					ONETOUCH_CONFIG_INIT_SCRAPER,
					{
						{
							{ 1, Utility::ScreenDataPageTypes::Page_OneTouch },				// ONETOUCH -> (line down) -> ONETOUCH
							{ 2, Utility::ScreenDataPageTypes::Page_OneTouch },				// ONETOUCH -> (select) -> HOME
							{ 3, Utility::ScreenDataPageTypes::Page_EquipmentOnOff },		// HOME -> (select) -> EQUIPMENT STATUS
							{ 4, Utility::ScreenDataPageTypes::Page_EquipmentOnOff },		// EQUIPMENT STATUS -> (page down) -> EQUIPMENT STATUS
							{ 5, Utility::ScreenDataPageTypes::Page_EquipmentOnOff },		// EQUIPMENT STATUS -> (page down) -> EQUIPMENT STATUS
							{ 6, Utility::ScreenDataPageTypes::Page_Home },					// EQUIPMENT STATUS -> (back) -> HOME
							{ 7, Utility::ScreenDataPageTypes::Page_Home },					// HOME -> (line down) -> HOME
							{ 8, Utility::ScreenDataPageTypes::Page_Home },					// HOME -> (line down) -> HOME
							{ 9, Utility::ScreenDataPageTypes::Page_MenuHelp },				// HOME -> (select) -> MENU
							{ 10, Utility::ScreenDataPageTypes::Page_MenuHelp },			// MENU -> (select) -> HELP
							{ 11, Utility::ScreenDataPageTypes::Page_MenuHelp },			// HELP -> (line down) -> HELP
							{ 12, Utility::ScreenDataPageTypes::Page_MenuHelp },			// HELP -> (line down) -> HELP
							{ 13, Utility::ScreenDataPageTypes::Page_DiagnosticsSensors },	// HELP -> (select) -> DIAGNOSTICS
							{ 14, Utility::ScreenDataPageTypes::Page_DiagnosticsRemotes },	// DIAGNOSTICS -> (select) -> DIAGNOSTICS
							{ 15, Utility::ScreenDataPageTypes::Page_DiagnosticsErrors },	// DIAGNOSTICS -> (select) -> DIAGNOSTICS 
							{ 16, Utility::ScreenDataPageTypes::Page_MenuHelp },			// DIAGNOSTICS -> (select) -> HELP
							{ 17, Utility::ScreenDataPageTypes::Page_MenuHelp },			// HELP -> (back) -> MENU
							{ 18, Utility::ScreenDataPageTypes::Page_Home },				// MENU -> (back) -> HOME
							{ 19, Utility::ScreenDataPageTypes::Page_Unknown }				// ...end
						},
						{
							{ 1, 2, { 1, KeyCommands::LineDown }},
							{ 2, 3, { 2, KeyCommands::Select }},
							{ 3, 4, { 3, KeyCommands::Select }},
							{ 4, 5, { 4, KeyCommands::PageDown_Or_Select1 }},
							{ 5, 6, { 5, KeyCommands::PageDown_Or_Select1 }},
							{ 6, 7, { 6, KeyCommands::Back_Or_Select2 }},
							{ 7, 8, { 7, KeyCommands::LineDown }},
							{ 8, 9, { 8, KeyCommands::LineDown }},
							{ 9, 10, { 9, KeyCommands::Select }},
							{ 10, 11, { 10, KeyCommands::Select }},
							{ 11, 12, { 11, KeyCommands::LineDown }},
							{ 12, 13, { 12, KeyCommands::LineDown }},
							{ 13, 14, { 13, KeyCommands::Select }},
							{ 14, 15, { 14, KeyCommands::Select }},
							{ 15, 16, { 15, KeyCommands::Select }},
							{ 16, 17, { 16, KeyCommands::Select }},
							{ 17, 18, { 17, KeyCommands::Back_Or_Select2 }},
							{ 18, 19, { 18, KeyCommands::Back_Or_Select2 }}
						}
					}
				}
			},
			JandyMessage_Status{}
		),
		Capabilities::Emulated(is_emulated),
		m_ProfilingDomain(std::move(Factory::ProfilingUnitFactory::Instance().CreateDomain("OneTouchDevice")))
	{
		m_ProfilingDomain->Start();

		PageProcessors(
			{
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Home, { 9, "Equipment ON/OFF" }, std::bind(&OneTouchDevice::PageProcessor_Home, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Service, { 3, "Service Mode" }, std::bind(&OneTouchDevice::PageProcessor_Service, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_TimeOut, { 3, "Timeout Mode" }, std::bind(&OneTouchDevice::PageProcessor_TimeOut, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_OneTouch, { 11, "SYSTEM" }, std::bind(&OneTouchDevice::PageProcessor_OneTouch, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_System, { 4, "Jandy AquaLinkRS" }, std::bind(&OneTouchDevice::PageProcessor_System, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_EquipmentOnOff, { 11, "More" }, std::bind(&OneTouchDevice::PageProcessor_EquipmentOnOff, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_EquipmentStatus, { 0, "EQUIPMENT STATUS" }, std::bind(&OneTouchDevice::PageProcessor_EquipmentStatus, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SelectSpeed, { 0, "Select Speed" }, std::bind(&OneTouchDevice::PageProcessor_SelectSpeed, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_MenuHelp, { 0, "Menu" }, std::bind(&OneTouchDevice::PageProcessor_MenuHelp, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetTemperature, { 0, "Set Temp" }, std::bind(&OneTouchDevice::PageProcessor_SetTemperature, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetTime, { 0, "Set Time" }, std::bind(&OneTouchDevice::PageProcessor_SetTime, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SystemSetup, { 0, "System Setup" }, std::bind(&OneTouchDevice::PageProcessor_SystemSetup, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_FreezeProtect, { 0, "Freeze Protect" }, std::bind(&OneTouchDevice::PageProcessor_FreezeProtect, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Boost, { 0, "Boost Pool" }, std::bind(&OneTouchDevice::PageProcessor_Boost, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetAquapure, { 0, "Set AQUAPURE" }, std::bind(&OneTouchDevice::PageProcessor_SetAquapure, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Version, { 7, "REV " }, std::bind(&OneTouchDevice::PageProcessor_Version, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_DiagnosticsSensors, { 6, "Sensors" }, std::bind(&OneTouchDevice::PageProcessor_DiagnosticsSensors, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_DiagnosticsRemotes, { 0, "Remotes" }, std::bind(&OneTouchDevice::PageProcessor_DiagnosticsRemotes, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_DiagnosticsErrors, { 0, "Errors" }, std::bind(&OneTouchDevice::PageProcessor_DiagnosticsErrors, this, std::placeholders::_1))
			}
		);

		m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_MessageLong>(std::bind(&OneTouchDevice::Slot_OneTouch_MessageLong, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_Probe>(std::bind(&OneTouchDevice::Slot_OneTouch_Probe, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_Status>(std::bind(&OneTouchDevice::Slot_OneTouch_Status, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<PDAMessage_Clear>(std::bind(&OneTouchDevice::Slot_OneTouch_Clear, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<PDAMessage_Highlight>(std::bind(&OneTouchDevice::Slot_OneTouch_Highlight, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<PDAMessage_HighlightChars>(std::bind(&OneTouchDevice::Slot_OneTouch_HighlightChars, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<PDAMessage_ShiftLines>(std::bind(&OneTouchDevice::Slot_OneTouch_ShiftLines, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_Unknown>(std::bind(&OneTouchDevice::Slot_OneTouch_Unknown, this, std::placeholders::_1), device_id());

		if (!IsEmulated())
		{
			m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_Ack>(std::bind(&OneTouchDevice::Slot_OneTouch_Ack, this, std::placeholders::_1), device_id());
		}
	}

	OneTouchDevice::~OneTouchDevice()
	{
		m_ProfilingDomain->End();
	}

	void OneTouchDevice::ProcessControllerUpdates()
	{	
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("ProcessControllerUpdates -> Handle Operating State", BOOST_CURRENT_LOCATION);

		m_KeyCommand_ToSend = KeyCommands::NoKeyCommand;

		switch (m_OpState)
		{
		case OperatingStates::StartUp:
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("ProcessControllerUpdates -> Handle Operating State (Start Up)", BOOST_CURRENT_LOCATION);

			switch (DisplayedPageType())
			{
			case Utility::ScreenDataPageTypes::Page_OneTouch:
				LogDebug(Channel::Devices, "Emulated OneTouch device: scrape starting - initialising config (from OneTouch page)");
				m_OpState = OperatingStates::ColdStart;
				ScrapingStart(ONETOUCH_CONFIG_INIT_SCRAPER, ONETOUCH_COLD_START_SCRAPER_START_INDEX);
				break;

			case Utility::ScreenDataPageTypes::Page_Home:
				LogDebug(Channel::Devices, "Emulated OneTouch device: scrape starting - initialising config (from Home page)");
				m_OpState = OperatingStates::WarmStart;
				ScrapingStart(ONETOUCH_CONFIG_INIT_SCRAPER, ONETOUCH_WARM_START_SCRAPER_START_INDEX);
				break;

			default:
				// DO NOTHING HERE
				break;
			}
			break;
		}

		case OperatingStates::ColdStart:
			[[fallthrough]];
		case OperatingStates::WarmStart:
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("ProcessControllerUpdates -> Handle Operating State (Cold/Warm Start)", BOOST_CURRENT_LOCATION);

			auto scrape_step_outcome = ScrapingNext();
			if (!scrape_step_outcome.has_value())
			{
				switch (scrape_step_outcome.error())
				{
				case ErrorCodes::Scrapeable_ErrorCodes::WaitingForPage:
					LogTrace(Channel::Devices, "Emulated OneTouch device: scrape in-progress -> waiting on page");
					break;

				case ErrorCodes::Scrapeable_ErrorCodes::WaitingForMessage:
					LogTrace(Channel::Devices, "Emulated OneTouch device: scrape in-progress -> waiting for message");
					break;

				case ErrorCodes::Scrapeable_ErrorCodes::NoStepPossible:
					// NOTE: Flow was VERSION -> ONETOUCH/HOME -> [scraping] -> HOME
					LogInfo(Channel::Devices, std::format("Emulated OneTouch device initialisation ({}) complete -> entering normal operation", (OperatingStates::ColdStart == m_OpState) ? "COLD START" : "WARM START"));
					m_OpState = OperatingStates::NormalOperation;
					break;

				case ErrorCodes::Scrapeable_ErrorCodes::NoGraphBeingScraped:
					[[fallthrough]];
				default:
					// No scrape is active (or waiting) but it's a cold start...this is weird so force a transition to normal operation.
					LogDebug(Channel::Devices, std::format("Emulated OneTouch device initialisation ({}) in an abnormal state -> forcing entry to normal operation", (OperatingStates::ColdStart == m_OpState) ? "COLD START" : "WARM START"));
					m_OpState = OperatingStates::NormalOperation;
					break;
				}
			}
			else
			{
				try
				{
					m_KeyCommand_ToSend = std::any_cast<KeyCommands>(scrape_step_outcome.value());
					LogDebug(Channel::Devices, std::format("Emulated OneTouch device: scrape in-progress - sending next command: {}", magic_enum::enum_name(m_KeyCommand_ToSend)));
				}
				catch (const std::bad_any_cast& eAC)
				{
					LogDebug(Channel::Devices, std::format("Failed trying to get key command for next step: error -> {}", eAC.what()));
				}
			}
			break;
		}

		case OperatingStates::NormalOperation:
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("ProcessControllerUpdates -> Handle Operating State (Normal Operation)", BOOST_CURRENT_LOCATION);
			break;
		}

		case OperatingStates::FaultHasOccurred:
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("ProcessControllerUpdates -> Handle Operating State (Fault Occurred)", BOOST_CURRENT_LOCATION);
			break;
		}
		}

		Signal_AckMessage(m_AckType_ToSend, m_KeyCommand_ToSend);
	}

}
// namespace AqualinkAutomate::Devices
