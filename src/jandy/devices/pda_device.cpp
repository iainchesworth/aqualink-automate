#include <format>
#include <functional>

#include "logging/logging.h"
#include "jandy/devices/pda_device.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	PDADevice::PDADevice(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id) :
		PDADevice::PDADevice(io_context, device_id, JandyControllerOperatingModes::MonitorOnly)
	{
	}

	PDADevice::PDADevice(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, JandyControllerOperatingModes op_mode) :
		JandyController(io_context, device_id, PDA_TIMEOUT_DURATION, op_mode),
		m_DisplayedPage(PDA_PAGE_LINES),
		m_DisplayedPageUpdater(m_DisplayedPage),
		m_DisplayedPageProcessors
		{
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Home, {0, "   MAIN MENU    "}, std::bind(&PDADevice::PageProcessor_Home, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_EquipmentStatus, { 0, "EQUIPMENT STATUS" }, std::bind(&PDADevice::PageProcessor_EquipmentStatus, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetPoolHeat, { 0, "   POOL HEAT    " }, std::bind(&PDADevice::PageProcessor_PoolHeat, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetSpaHeat, { 0, "    SPA HEAT    " }, std::bind(&PDADevice::PageProcessor_SpaHeat, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetTemperature, { 0, "    SET TEMP    " }, std::bind(&PDADevice::PageProcessor_SetTemperature, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetTime, { 0, "    SET TIME    " }, std::bind(&PDADevice::PageProcessor_SetTime, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_FreezeProtect, { 0, " FREEZE PROTECT " }, std::bind(&PDADevice::PageProcessor_FreezeProtect, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Boost, { 0, "BOOST" }, std::bind(&PDADevice::PageProcessor_Boost, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_SetAquapure, { 0, "  SET AquaPure  " }, std::bind(&PDADevice::PageProcessor_AquaPure, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Version, { 3, "Firmware Version" }, std::bind(&PDADevice::PageProcessor_FirmwareVersion, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Version, { 1, "    AquaPalm" }, std::bind(&PDADevice::PageProcessor_FirmwareVersion, this, std::placeholders::_1)),
			Utility::ScreenDataPage_Processor(Utility::ScreenDataPageTypes::Page_Version, { 1, " PDA-P" }, std::bind(&PDADevice::PageProcessor_FirmwareVersion, this, std::placeholders::_1))
		}
	{
		m_DisplayedPageUpdater.initiate();

		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_Ack>(std::bind(&PDADevice::Slot_PDA_Ack, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::PDAMessage_Clear>(std::bind(&PDADevice::Slot_PDA_Clear, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::PDAMessage_Highlight>(std::bind(&PDADevice::Slot_PDA_Highlight, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::PDAMessage_HighlightChars>(std::bind(&PDADevice::Slot_PDA_HighlightChars, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_MessageLong>(std::bind(&PDADevice::Slot_PDA_MessageLong, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_Status>(std::bind(&PDADevice::Slot_PDA_Status, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::PDAMessage_ShiftLines>(std::bind(&PDADevice::Slot_PDA_ShiftLines, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_Unknown>(std::bind(&PDADevice::Slot_PDA_Unknown_PDA_1B, this, std::placeholders::_1), device_id());
	}

	PDADevice::PDADevice(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, JandyControllerOperatingModes op_mode, Config::JandyConfig& config) :
		PDADevice::PDADevice(io_context, device_id, op_mode)
	{
		InjectConfig(config);
	}

	PDADevice::~PDADevice()
	{
	}

}
// namespace AqualinkAutomate::Devices
