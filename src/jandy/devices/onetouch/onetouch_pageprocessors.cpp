#include <format>

#include "logging/logging.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/utility/string_manipulation.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	void OneTouchDevice::PageProcessor_Home(const Utility::ScreenDataPage& page)
	{
		/*
			Info:   OneTouch Menu Line 00 = Paddock Pools
			Info:   OneTouch Menu Line 01 =
			Info:   OneTouch Menu Line 02 =  01/18/11 Tue
			Info:   OneTouch Menu Line 03 =    7:13 PM
			Info:   OneTouch Menu Line 04 =
			Info:   OneTouch Menu Line 05 = Filter Pump OFF
			Info:   OneTouch Menu Line 06 =     Air 22`C
			Info:   OneTouch Menu Line 07 =
			Info:   OneTouch Menu Line 08 =
			Info:   OneTouch Menu Line 09 = Equipment ON/OFF
			Info:   OneTouch Menu Line 10 = OneTouch  ON/OFF
			Info:   OneTouch Menu Line 11 =    Menu / Help
		*/

		if (JandyController::m_Config.has_value())
		{
			auto& config_refwrap = JandyController::m_Config.value().get();
			config_refwrap.Mode = Equipment::JandyEquipmentModes::Normal;
			config_refwrap.AirTemp = Utility::TrimWhitespace(page[6]);
		}
	}

	void OneTouchDevice::PageProcessor_Service(const Utility::ScreenDataPage& page)
	{
		/*
			Info:   OneTouch Menu Line 00 =
			Info:   OneTouch Menu Line 01 =
			Info:   OneTouch Menu Line 02 =
			Info:   OneTouch Menu Line 03 =  Service Mode
			Info:   OneTouch Menu Line 04 =
			Info:   OneTouch Menu Line 05 = No  operations
			Info:   OneTouch Menu Line 06 =  allowed here
			Info:   OneTouch Menu Line 07 =
			Info:   OneTouch Menu Line 08 =
			Info:   OneTouch Menu Line 09 =
			Info:   OneTouch Menu Line 10 =
			Info:   OneTouch Menu Line 11 =
		*/

		if (JandyController::m_Config.has_value())
		{
			auto& config_refwrap = JandyController::m_Config.value().get();
			config_refwrap.Mode = Equipment::JandyEquipmentModes::Service;
		}
	}

	void OneTouchDevice::PageProcessor_TimeOut(const Utility::ScreenDataPage& page)
	{
		/*
			Info:   OneTouch Menu Line 00 =
			Info:   OneTouch Menu Line 01 =
			Info:   OneTouch Menu Line 02 =
			Info:   OneTouch Menu Line 03 =  Timeout Mode
			Info:   OneTouch Menu Line 04 =
			Info:   OneTouch Menu Line 05 = No  operations
			Info:   OneTouch Menu Line 06 =  allowed here
			Info:   OneTouch Menu Line 07 =
			Info:   OneTouch Menu Line 08 =
			Info:   OneTouch Menu Line 09 =
			Info:   OneTouch Menu Line 10 =    02:57:39
			Info:   OneTouch Menu Line 11 =
		*/

		if (JandyController::m_Config.has_value())
		{
			auto& config_refwrap = JandyController::m_Config.value().get();
			config_refwrap.Mode = Equipment::JandyEquipmentModes::TimeOut;
			config_refwrap.TimeoutRemaining = Utility::TimeoutDuration(Utility::TrimWhitespace(page[10]));
		}
	}

	void OneTouchDevice::PageProcessor_OneTouch(const Utility::ScreenDataPage& page)
	{
	}

	void OneTouchDevice::PageProcessor_System(const Utility::ScreenDataPage& page)
	{
	}

	void OneTouchDevice::PageProcessor_EquipmentStatus(const Utility::ScreenDataPage& page)
	{
		/*
			Info:   OneTouch Menu Line 00 = Equipment Status		Equipment Status	Equipment Status
			Info:   OneTouch Menu Line 01 =
			Info:   OneTouch Menu Line 02 = Intelliflo VS 3		Intelliflo VS 3		Intelliflo VF 2
			Info:   OneTouch Menu Line 03 =  *** Priming ***		     RPM: 2750		     RPM: 2250
			Info:   OneTouch Menu Line 04 =     Watts: 100             RPM: 600		    Watts: 55
			Info:   OneTouch Menu Line 05 =                          Watts: 55		      GPM: 80
			Info:   OneTouch Menu Line 06 =
			Info:   OneTouch Menu Line 07 =
			Info:   OneTouch Menu Line 08 =
			Info:   OneTouch Menu Line 09 =
			Info:   OneTouch Menu Line 10 =
			Info:   OneTouch Menu Line 11 =
		*/
	}

	void OneTouchDevice::PageProcessor_SelectSpeed(const Utility::ScreenDataPage& page)
	{
	}

	void OneTouchDevice::PageProcessor_MenuHelp(const Utility::ScreenDataPage& page)
	{
	}

	void OneTouchDevice::PageProcessor_SetTemperature(const Utility::ScreenDataPage& page)
	{
		/*
			Info:   OneTouch Menu Line 00 =     Set Temp
			Info:   OneTouch Menu Line 01 =
			Info:   OneTouch Menu Line 02 = Pool Heat   90`F
			Info:   OneTouch Menu Line 03 = Spa Heat   102`F
			Info:   OneTouch Menu Line 04 =
			Info:   OneTouch Menu Line 05 = Maintain     OFF
			Info:   OneTouch Menu Line 06 = Hours  12AM-12AM
			Info:   OneTouch Menu Line 07 =
			Info:   OneTouch Menu Line 08 = Highlight an
			Info:   OneTouch Menu Line 09 = item and press
			Info:   OneTouch Menu Line 10 = Select
			Info:   OneTouch Menu Line 11 =
		*/


		if (JandyController::m_Config.has_value())
		{
			auto& config_refwrap = JandyController::m_Config.value().get();
			config_refwrap.PoolTemp = Utility::TrimWhitespace(page[2]);
			config_refwrap.SpaTemp = Utility::TrimWhitespace(page[3]);
			auto is_maintained = Utility::TrimWhitespace(page[5]);
			auto maintenance_hours = Utility::TrimWhitespace(page[6]);
		}
	}

	void OneTouchDevice::PageProcessor_SetTime(const Utility::ScreenDataPage& page)
	{
	}

	void OneTouchDevice::PageProcessor_SystemSetup(const Utility::ScreenDataPage& page)
	{
	}

	void OneTouchDevice::PageProcessor_FreezeProtect(const Utility::ScreenDataPage& page)
	{
		/*
			Info:   OneTouch Menu Line 00 =  Freeze Protect
			Info:   OneTouch Menu Line 01 =
			Info:   OneTouch Menu Line 02 =
			Info:   OneTouch Menu Line 03 = Temp        38`F
			Info:   OneTouch Menu Line 04 =
			Info:   OneTouch Menu Line 05 =
			Info:   OneTouch Menu Line 06 = Use Arrow Keys
			Info:   OneTouch Menu Line 07 = to set value.
			Info:   OneTouch Menu Line 08 = Press SELECT
			Info:   OneTouch Menu Line 09 = to continue.
			Info:   OneTouch Menu Line 10 =
			Info:   OneTouch Menu Line 11 =
		*/

		if (JandyController::m_Config.has_value())
		{
			auto& config_refwrap = JandyController::m_Config.value().get();
			config_refwrap.FreezeProtectPoint = Utility::TrimWhitespace(page[3]);
		}
	}

	void OneTouchDevice::PageProcessor_Boost(const Utility::ScreenDataPage& page)
	{
	}

	void OneTouchDevice::PageProcessor_SetAquapure(const Utility::ScreenDataPage& page)
	{
	}

	void OneTouchDevice::PageProcessor_Version(const Utility::ScreenDataPage& page)
	{
		/*
			Info:   OneTouch Menu Line 00 =
			Info:   OneTouch Menu Line 01 =
			Info:   OneTouch Menu Line 02 =
			Info:   OneTouch Menu Line 03 =
			Info:   OneTouch Menu Line 04 =     B0029221
			Info:   OneTouch Menu Line 05 =    RS-8 Combo
			Info:   OneTouch Menu Line 06 =
			Info:   OneTouch Menu Line 07 =    REV T.0.1
			Info:   OneTouch Menu Line 08 =
			Info:   OneTouch Menu Line 09 =
			Info:   OneTouch Menu Line 10 =
			Info:   OneTouch Menu Line 11 =
		*/

		const auto serial_number = Utility::TrimWhitespace(page[4]);
		const auto panel_type = Utility::TrimWhitespace(page[5]);
		const auto panel_type_converted = Equipment::JandyEquipmentType_FromString(panel_type);
		const auto fw_revision = Utility::TrimWhitespace(page[7]);

		if (JandyController::m_Config.has_value())
		{
			auto& config_refwrap = JandyController::m_Config.value().get();
			config_refwrap.EquipmentVersions.SerialNumber = serial_number;
			config_refwrap.EquipmentVersions.PanelType = panel_type_converted;
			config_refwrap.EquipmentVersions.FirmwareRevision = fw_revision;
		}

		LogInfo(Channel::Devices, std::format("Aqualink Power Center - Serial: {}, Type: {}, Rev: {}", serial_number, panel_type, fw_revision));
	}

	void OneTouchDevice::PageProcessor_DiagnosticsSensors(const Utility::ScreenDataPage& page)
	{
		/*
			Info:   OneTouch Menu Line 00 = Model   B0029221
			Info:   OneTouch Menu Line 01 = Type  RS-8 Combo
			Info:   OneTouch Menu Line 02 = Firmware   T.0.1
			Info:   OneTouch Menu Line 03 =
			Info:   OneTouch Menu Line 04 =     
			Info:   OneTouch Menu Line 05 = 
			Info:   OneTouch Menu Line 06 =     Sensors
			Info:   OneTouch Menu Line 07 = Water         OK
			Info:   OneTouch Menu Line 08 = Air           OK
			Info:   OneTouch Menu Line 09 = Solar		  OK
			Info:   OneTouch Menu Line 10 =
			Info:   OneTouch Menu Line 11 =       Next 
		*/
	}

	void OneTouchDevice::PageProcessor_DiagnosticsRemotes(const Utility::ScreenDataPage& page)
	{
		/*
			Info:   OneTouch Menu Line 00 =     Remotes
			Info:   OneTouch Menu Line 01 = 
			Info:   OneTouch Menu Line 02 = OneTouch       4
			Info:   OneTouch Menu Line 03 = LX Htr         1
			Info:   OneTouch Menu Line 04 =                1
			Info:   OneTouch Menu Line 05 = (Not Compatible)
			Info:   OneTouch Menu Line 06 = Spa Sw Board   1
			Info:   OneTouch Menu Line 07 = 
			Info:   OneTouch Menu Line 08 = 
			Info:   OneTouch Menu Line 09 = 
			Info:   OneTouch Menu Line 10 =
			Info:   OneTouch Menu Line 11 =       Next
		*/
	}

	void OneTouchDevice::PageProcessor_DiagnosticsErrors(const Utility::ScreenDataPage& page)
	{
		/*
			Info:   OneTouch Menu Line 00 =      Errors              Errors
			Info:   OneTouch Menu Line 01 = 
			Info:   OneTouch Menu Line 02 = 
			Info:   OneTouch Menu Line 03 =                     Heater   Gas Sw
			Info:   OneTouch Menu Line 04 =
			Info:   OneTouch Menu Line 05 =    No Errors   
			Info:   OneTouch Menu Line 06 =     
			Info:   OneTouch Menu Line 07 = 
			Info:   OneTouch Menu Line 08 = 
			Info:   OneTouch Menu Line 09 = 
			Info:   OneTouch Menu Line 10 =
			Info:   OneTouch Menu Line 11 =    Continue            Continue
		*/
	}

}
// namespace AqualinkAutomate::Devices
