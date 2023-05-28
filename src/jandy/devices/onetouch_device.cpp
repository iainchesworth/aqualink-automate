#include <functional>
#include <ranges>

#include "logging/logging.h"
#include "jandy/devices/onetouch_device.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	OneTouchDevice::OneTouchDevice(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, Config::JandyConfig& config, JandyControllerOperatingModes op_mode) :
		JandyController(io_context, device_id, ONETOUCH_TIMEOUT_DURATION, config, op_mode),
		m_DisplayedPage(ONETOUCH_PAGE_LINES),
		m_DisplayedPageUpdater(m_DisplayedPage),
		m_DisplayedPageProcessors
		{
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Home, { 9, "Equipment ON/OFF" }, std::bind(&OneTouchDevice::PageProcessor_Home, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Service, { 3, "Service Mode" }, std::bind(&OneTouchDevice::PageProcessor_Service, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_TimeOut, { 3, "Timeout Mode" }, std::bind(&OneTouchDevice::PageProcessor_TimeOut, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_OneTouch, { 11, "SYSTEM" }, std::bind(&OneTouchDevice::PageProcessor_OneTouch, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_System, { 4, "Jandy AquaLinkRS" }, std::bind(&OneTouchDevice::PageProcessor_System, this, std::placeholders::_1)),
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
		},
		m_InitialisationGraph
		{
			{
				{ 1, Utility::ScreenDataPageTypes::Page_OneTouch },				// ONETOUCH -> (line down) -> ONETOUCH
				{ 2, Utility::ScreenDataPageTypes::Page_OneTouch },				// ONETOUCH -> (select) -> HOME
				{ 3, Utility::ScreenDataPageTypes::Page_EquipmentStatus },		// HOME -> (select) -> EQUIPMENT STATUS
				{ 4, Utility::ScreenDataPageTypes::Page_EquipmentStatus },		// EQUIPMENT STATUS -> (page down) -> EQUIPMENT STATUS
				{ 5, Utility::ScreenDataPageTypes::Page_EquipmentStatus },		// EQUIPMENT STATUS -> (page down) -> EQUIPMENT STATUS
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
		},
		m_InitialisationGraphIterator(Utility::ScreenDataPageGraphImpl::ForwardIterator::begin(m_InitialisationGraph, 1))
	{
		m_DisplayedPageUpdater.initiate();

		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_MessageLong>(std::bind(&OneTouchDevice::Slot_OneTouch_MessageLong, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_Probe>(std::bind(&OneTouchDevice::Slot_OneTouch_Probe, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_Status>(std::bind(&OneTouchDevice::Slot_OneTouch_Status, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::PDAMessage_Clear>(std::bind(&OneTouchDevice::Slot_OneTouch_Clear, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::PDAMessage_Highlight>(std::bind(&OneTouchDevice::Slot_OneTouch_Highlight, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::PDAMessage_HighlightChars>(std::bind(&OneTouchDevice::Slot_OneTouch_HighlightChars, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::PDAMessage_ShiftLines>(std::bind(&OneTouchDevice::Slot_OneTouch_ShiftLines, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_Unknown>(std::bind(&OneTouchDevice::Slot_OneTouch_Unknown, this, std::placeholders::_1), device_id());

		if (JandyControllerOperatingModes::MonitorOnly == m_OpMode)
		{
			m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_Ack>(std::bind(&OneTouchDevice::Slot_OneTouch_Ack, this, std::placeholders::_1), device_id());
		}
	}

	OneTouchDevice::~OneTouchDevice()
	{
	}

	void OneTouchDevice::HandleAnyInternalProcessing()
	{
		switch (m_OpState)
		{
		case OperatingStates::StartUp:
			break;

		case OperatingStates::InitComplete:
			break;

		case OperatingStates::NormalOperation:
			break;

		case OperatingStates::FaultHasOccurred:
			break;
		}
	}

	void OneTouchDevice::HandleAnyScreenProcessing()
	{
		switch (m_ScreenMode)
		{
		case ScreenModes::Normal:
			break;

		case ScreenModes::Updating:
			break;

		case ScreenModes::UpdateComplete:
		{
			// Process the "page" to extract information.
			auto actionable_processors = m_DisplayedPageProcessors | std::views::filter([this](const decltype(m_DisplayedPageProcessors)::value_type& processor) { return processor.CanProcess(m_DisplayedPage); });
			for (auto& processor : actionable_processors)
			{
				processor.Process(m_DisplayedPage);
			}
			m_ScreenMode = ScreenModes::Normal;
			break;
		}
		}
	}

}
// namespace AqualinkAutomate::Devices
