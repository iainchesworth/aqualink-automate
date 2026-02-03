#include <functional>

#include <magic_enum/magic_enum.hpp>

#include "logging/logging.h"
#include "devices/device_status.h"
#include "devices/onetouch_device.h"
#include "formatters/jandy_device_formatters.h"
#include "utility/screen_data_page_processor.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Messages;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Devices
{

	OneTouchDevice::OneTouchDevice(std::shared_ptr<Devices::JandyDeviceType> device_id, Kernel::HubLocator& hub_locator, bool is_emulated) :
		JandyController(device_id, hub_locator),
		Capabilities::Restartable(ONETOUCH_TIMEOUT_DURATION),
		Capabilities::Screen(ONETOUCH_PAGE_LINES),
		Capabilities::Scrapeable
		(
			*device_id,
			{
				{ ONETOUCH_AUX_LABELS_NAV_SCRAPER, ONETOUCH_AUX_LABELS_NAV_SCRAPER_GRAPH },
				{ ONETOUCH_AUX_LABELS_TEXT_SCRAPER, ONETOUCH_AUX_LABELS_TEXT_SCRAPER_GRAPH },
				{ ONETOUCH_CONFIG_INIT_SCRAPER, ONETOUCH_CONFIG_INIT_SCRAPER_GRAPH }
			},
			JandyMessage_Status{}
		),
		Capabilities::Emulated(is_emulated),
		m_StartUpScrapeGraphsIt(STARTUP_SCRAPE_GRAPHS.cbegin()),
		m_ProfilingDomain(std::move(Factory::ProfilingUnitFactory::Instance().CreateDomain("OneTouchDevice")))
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::OneTouchDevice", std::source_location::current());

		LogInfo(Channel::Devices, std::format("Creating OneTouchDevice: device_id={}, emulated={}, timeout={}s", *device_id, is_emulated, ONETOUCH_TIMEOUT_DURATION.count()));

		m_ProfilingDomain->Start();

		PageProcessors(
			{
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Home, { 9, "Equipment ON/OFF" }, std::bind(&OneTouchDevice::PageProcessor_Home, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Service, { 3, "Service Mode" }, std::bind(&OneTouchDevice::PageProcessor_Service, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_TimeOut, { 3, "Timeout Mode" }, std::bind(&OneTouchDevice::PageProcessor_TimeOut, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_OneTouch, { 11, "SYSTEM" }, std::bind(&OneTouchDevice::PageProcessor_OneTouch, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_System, { 4, "Jandy AquaLinkRS" }, std::bind(&OneTouchDevice::PageProcessor_System, this, std::placeholders::_1)),
				Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_EquipmentOnOff, { 11, "More" }, std::bind(&OneTouchDevice::PageProcessor_EquipmentOnOff, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_EquipmentOnOff, { 0, "Filter Pump" }, std::bind(&OneTouchDevice::PageProcessor_EquipmentOnOff, this, std::placeholders::_1)),
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
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::ProcessControllerUpdates", std::source_location::current());

		LogTrace(Channel::Devices, std::format("OneTouch ({}): ProcessControllerUpdates called: state={}", DeviceId(), magic_enum::enum_name(m_OpState)));

		m_KeyCommand_ToSend = KeyCommands::NoKeyCommand;

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

		case OperatingStates::ColdStart:
			[[fallthrough]];
		case OperatingStates::WarmStart:
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::ProcessControllerUpdates -> cold_warm_start", std::source_location::current());
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Processing {} state", DeviceId(), magic_enum::enum_name(m_OpState)));
			Scraping_ProcessStep_ColdAndWarmStart();
			break;
		}

		case OperatingStates::NormalOperation:
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::ProcessControllerUpdates -> normal_operation", std::source_location::current());
			LogTrace(Channel::Devices, std::format("OneTouch ({}): Processing NormalOperation state", DeviceId()));
			break;
		}

		case OperatingStates::FaultHasOccurred:
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::ProcessControllerUpdates -> fault", std::source_location::current());
			LogWarning(Channel::Devices, std::format("OneTouch ({}): Processing FaultHasOccurred state", DeviceId()));
			break;
		}
		}

		if (m_KeyCommand_ToSend != KeyCommands::NoKeyCommand)
		{
			LogTrace(Channel::Devices, std::format("OneTouch({}) : Sending key command: {}", DeviceId(), magic_enum::enum_name(m_KeyCommand_ToSend)));
		}

		Signal_AckMessage(m_AckType_ToSend, m_KeyCommand_ToSend);
	}

	void OneTouchDevice::WatchdogTimeoutOccurred()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::WatchdogTimeoutOccurred", std::source_location::current());

		LogWarning(Channel::Devices, std::format("OneTouch({}) : Watchdog timeout occurred: state={}, timeout_duration={}s", DeviceId(), magic_enum::enum_name(m_OpState), ONETOUCH_TIMEOUT_DURATION.count()));

		if (m_OpState == OperatingStates::ColdStart || m_OpState == OperatingStates::WarmStart)
		{
			// Scraping was in progress when the watchdog fired.  Abandon the
			// scraping sequence and fall through to normal (passive) operation
			// so the device is at least partially functional.
			LogWarning(Channel::Devices, std::format("OneTouch({}) : Abandoning {} scraping due to watchdog timeout -> entering NormalOperation", DeviceId(), magic_enum::enum_name(m_OpState)));
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

}
// namespace AqualinkAutomate::Devices
