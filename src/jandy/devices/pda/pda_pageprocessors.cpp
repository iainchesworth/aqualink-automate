#include <format>

#include "logging/logging.h"
#include "jandy/devices/pda_device.h"
#include "jandy/utility/string_manipulation.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	void PDADevice::PageProcessor_Home(const Utility::ScreenDataPage& page)
	{
		LogDebug(Channel::Devices, "PDA device is processing a PageProcessor_Home page.");

		/*
			Info:   PDA Menu Line 00 =
			Info:   PDA Menu Line 01 = Air 
			Info:   PDA Menu Line 02 =
			Info:   PDA Menu Line 03 =
			Info:   PDA Menu Line 04 = Pool Mode    OFF
			Info:   PDA Menu Line 05 = Pool Heater  OFF
			Info:   PDA Menu Line 06 = Spa Mode     OFF
			Info:   PDA Menu Line 07 = Spa Heater   OFF
			Info:   PDA Menu Line 08 = Menu
			Info:   PDA Menu Line 09 = Equipment ON/OFF
		*/
	}

	void PDADevice::PageProcessor_SetTemperature(const Utility::ScreenDataPage& page)
	{
	}

	void PDADevice::PageProcessor_SetTime(const Utility::ScreenDataPage& page)
	{
	}

	void PDADevice::PageProcessor_PoolHeat(const Utility::ScreenDataPage& page)
	{
	}

	void PDADevice::PageProcessor_SpaHeat(const Utility::ScreenDataPage& page)
	{
	}

	void PDADevice::PageProcessor_AquaPure(const Utility::ScreenDataPage& page)
	{
	}

	void PDADevice::PageProcessor_AuxLabel(const Utility::ScreenDataPage& page)
	{
	}

	void PDADevice::PageProcessor_FreezeProtect(const Utility::ScreenDataPage& page)
	{
	}

	void PDADevice::PageProcessor_Settings(const Utility::ScreenDataPage& page)
	{
	}

	void PDADevice::PageProcessor_EquipmentStatus(const Utility::ScreenDataPage& page)
	{
	}

	void PDADevice::PageProcessor_Boost(const Utility::ScreenDataPage& page)
	{
	}
	
	void PDADevice::PageProcessor_FirmwareVersion(const Utility::ScreenDataPage& page)
	{
		LogDebug(Channel::Devices, "PDA device is processing a PageProcessor_FirmwareVersion page.");

		/*
			Info:   PDA Menu Line 00 =
			Info:   PDA Menu Line 01 =      AquaPalm
			Info:   PDA Menu Line 02 =
			Info:   PDA Menu Line 03 =  Firmware Version
			Info:   PDA Menu Line 04 =
			Info:   PDA Menu Line 05 =    
			Info:   PDA Menu Line 06 =     REV T.0.1
			Info:   PDA Menu Line 07 =
			Info:   PDA Menu Line 08 =
			Info:   PDA Menu Line 09 =
		*/

		const auto model_number = Utility::TrimWhitespace(page[1].Text);
		const auto fw_revision = Utility::TrimWhitespace(page[6].Text);

		JandyController::m_Config.EquipmentVersions.ModelNumber = model_number;
		JandyController::m_Config.EquipmentVersions.FirmwareRevision = fw_revision;

		LogInfo(Channel::Devices, std::format("Aqualink Power Center - Model: {}, Rev: {}", model_number, fw_revision));
	}

}
// namespace AqualinkAutomate::Devices
