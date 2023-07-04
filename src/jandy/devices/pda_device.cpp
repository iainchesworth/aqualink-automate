#include <format>
#include <functional>

#include "logging/logging.h"
#include "jandy/devices/pda_device.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Messages;

namespace AqualinkAutomate::Devices
{
	
	PDADevice::PDADevice(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, Kernel::DataHub& config, bool is_emulated) :
		JandyController(io_context, device_id, PDA_TIMEOUT_DURATION, config),
		Capabilities::Screen(PDA_PAGE_LINES),
		Capabilities::Scrapeable
		(
			device_id,
			{
				{ PDA_CONFIG_INIT_SCRAPER + 0, { {},{} } },
				{ PDA_CONFIG_INIT_SCRAPER + 1, { {},{} } }
			},
			JandyMessage_Status{}
		),
		Capabilities::Emulated(is_emulated)
		
	{
		PageProcessors(
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
		);

		m_SlotManager.RegisterSlot_FilterByDeviceId<PDAMessage_Clear>(std::bind(&PDADevice::Slot_PDA_Clear, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<PDAMessage_Highlight>(std::bind(&PDADevice::Slot_PDA_Highlight, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<PDAMessage_HighlightChars>(std::bind(&PDADevice::Slot_PDA_HighlightChars, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_Probe>(std::bind(&PDADevice::Slot_PDA_Probe, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_MessageLong>(std::bind(&PDADevice::Slot_PDA_MessageLong, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_Status>(std::bind(&PDADevice::Slot_PDA_Status, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<PDAMessage_ShiftLines>(std::bind(&PDADevice::Slot_PDA_ShiftLines, this, std::placeholders::_1), device_id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_Unknown>(std::bind(&PDADevice::Slot_PDA_Unknown_PDA_1B, this, std::placeholders::_1), device_id());

		if (!IsEmulated())
		{
			m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_Ack>(std::bind(&PDADevice::Slot_PDA_Ack, this, std::placeholders::_1), device_id());
		}
	}

	PDADevice::~PDADevice()
	{
	}
	
	void PDADevice::ProcessControllerUpdates()
	{
	}

}
// namespace AqualinkAutomate::Devices
