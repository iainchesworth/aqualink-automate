#include <functional>
#include <ranges>

#include "logging/logging.h"
#include "jandy/devices/onetouch_device.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	OneTouchDevice::OneTouchDevice(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, JandyDeviceOperatingModes op_mode) :
		JandyDevice(io_context, device_id, ONETOUCH_TIMEOUT_DURATION, op_mode),
		m_DisplayedPage(ONETOUCH_PAGE_LINES),
		m_DisplayedPageUpdater(m_DisplayedPage),
		m_DisplayedPageProcessors
		{
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Home, { 9, "Equipment ON/OFF" }, std::bind(&OneTouchDevice::PageProcessor_Home, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_OneTouch, { 11, "SYSTEM" }, std::bind(&OneTouchDevice::PageProcessor_OneTouch, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_System, { 0, "Jandy AquaLinkRS" }, std::bind(&OneTouchDevice::PageProcessor_System, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_EquipmentStatus, { 0, "EQUIPMENT STATUS" }, std::bind(&OneTouchDevice::PageProcessor_EquipmentStatus, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SelectSpeed, { 0, "Select Speed" }, std::bind(&OneTouchDevice::PageProcessor_SelectSpeed, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_MenuHelp, { 0, "Menu" }, std::bind(&OneTouchDevice::PageProcessor_MenuHelp, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetTemperature, { 0, "Set Temp" }, std::bind(&OneTouchDevice::PageProcessor_SetTemperature, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetTime, { 0, "Set Time" }, std::bind(&OneTouchDevice::PageProcessor_SetTime, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SystemSetup, { 0, "System Setup" }, std::bind(&OneTouchDevice::PageProcessor_SystemSetup, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_FreezeProtect, { 0, "Freeze Protect" }, std::bind(&OneTouchDevice::PageProcessor_FreezeProtect, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Boost, { 0, "Boost Pool" }, std::bind(&OneTouchDevice::PageProcessor_Boost, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetAquapure, { 0, "Set AQUAPURE" }, std::bind(&OneTouchDevice::PageProcessor_SetAquapure, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Version, { 7, "REV " }, std::bind(&OneTouchDevice::PageProcessor_Version, this, std::placeholders::_1))
		}
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

		case OperatingStates::ScreenUpdating:
		{
			// Process the "page" to extract information.
			auto actionable_processors = m_DisplayedPageProcessors | std::views::filter([this](const decltype(m_DisplayedPageProcessors)::value_type& processor) { return processor.CanProcess(m_DisplayedPage); });
			for (auto& processor : actionable_processors)
			{
				processor.Process(m_DisplayedPage);
			}
			m_OpState = OperatingStates::NormalOperation;
			break;
		}

		case OperatingStates::FaultHasOccurred:
			break;
		}
	}

}
// namespace AqualinkAutomate::Devices
