#include <execution>
#include <functional>
#include <format>
#include <memory>
#include <tuple>

#include <re2/re2.h>

#include "logging/logging.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/factories/jandy_auxillary_factory.h"
#include "jandy/utility/jandy_pool_configuration_decoder.h"
#include "jandy/utility/string_manipulation.h"
#include "jandy/utility/string_conversion/auxillary_state.h"
#include "jandy/utility/string_conversion/temperature.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	void OneTouchDevice::PageProcessor_Home(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_Home", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_Home page.");

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

		JandyController::m_Config.Mode = Equipment::JandyEquipmentModes::Normal;

		///FIXME - get pool / spa temperature if FP pump is running....

		if (auto temperature = Utility::Temperature(Utility::TrimWhitespace(page[6].Text)); temperature().has_value())
		{
			JandyController::m_Config.AirTemp(temperature().value());
		}
	}

	void OneTouchDevice::PageProcessor_Service(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_Service", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_Service page.");

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

		JandyController::m_Config.Mode = Equipment::JandyEquipmentModes::Service;
	}

	void OneTouchDevice::PageProcessor_TimeOut(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_TimeOut", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_TimeOut page.");

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

		JandyController::m_Config.Mode = Equipment::JandyEquipmentModes::TimeOut;
		JandyController::m_Config.TimeoutRemaining = Utility::TimeoutDuration(Utility::TrimWhitespace(page[10].Text));
	}

	void OneTouchDevice::PageProcessor_OneTouch(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_OneTouch", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_OneTouch page.");

		/*
			Info:   OneTouch Menu Line 00 =
			Info:   OneTouch Menu Line 01 = All Off
			Info:   OneTouch Menu Line 02 =
			Info:   OneTouch Menu Line 03 = 
			Info:   OneTouch Menu Line 04 = Spa Mode     OFF
			Info:   OneTouch Menu Line 05 = 
			Info:   OneTouch Menu Line 06 = 
			Info:   OneTouch Menu Line 07 = Clean Mode   OFF
			Info:   OneTouch Menu Line 08 =
			Info:   OneTouch Menu Line 09 =
			Info:   OneTouch Menu Line 10 =  More OneTouch
			Info:   OneTouch Menu Line 11 =      System
		*/
	}

	void OneTouchDevice::PageProcessor_System(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_System", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_System page.");
	}
	
	void OneTouchDevice::PageProcessor_EquipmentOnOff(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_EquipmentOnOff", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_EquipmentOnOff page.");

		/*
			Info:   OneTouch Menu Line 00 = Filter Pump  ***
			Info:   OneTouch Menu Line 01 = Spa           ON
			Info:   OneTouch Menu Line 02 = Pool Heat    ENA
			Info:   OneTouch Menu Line 03 = Spa Heat     OFF
			Info:   OneTouch Menu Line 04 = Solar Heat   OFF
			Info:   OneTouch Menu Line 05 = Aux1         OFF
			Info:   OneTouch Menu Line 06 = Aux2         OFF
			Info:   OneTouch Menu Line 07 = Aux3         OFF
			Info:   OneTouch Menu Line 08 = Aux4         OFF
			Info:   OneTouch Menu Line 09 = Aux5         OFF
			Info:   OneTouch Menu Line 10 = Aux6         OFF
			Info:   OneTouch Menu Line 11 =    ^^ More vv
		*/

		for (uint8_t row_index = 0; row_index < (page.Size() - 1); row_index++)
		{
			auto new_aux_state = Utility::AuxillaryState(Utility::TrimWhitespace(page[row_index].Text));
			auto aux_ptr = Factory::JandyAuxillaryFactory::Instance().CreateDevice(new_aux_state);
			JandyController::m_Config.Devices.Add(aux_ptr);
		}
	}

	void OneTouchDevice::PageProcessor_EquipmentStatus(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_EquipmentStatus", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_EquipmentStatus page.");

		/*
			Info:   OneTouch Menu Line 00 = Equipment Status    Equipment Status	Equipment Status
			Info:   OneTouch Menu Line 01 =
			Info:   OneTouch Menu Line 02 = Intelliflo VS 3		Intelliflo VS 3		Intelliflo VF 2
			Info:   OneTouch Menu Line 03 =  *** Priming ***		   RPM: 2750		  RPM: 2250
			Info:   OneTouch Menu Line 04 =     Watts: 100           Watts: 600		    Watts: 55
			Info:   OneTouch Menu Line 05 =                            GPM: 55		      GPM: 80
			Info:   OneTouch Menu Line 06 =
			Info:   OneTouch Menu Line 07 =
			Info:   OneTouch Menu Line 08 =
			Info:   OneTouch Menu Line 09 =
			Info:   OneTouch Menu Line 10 =
			Info:   OneTouch Menu Line 11 =
		*/

		using ScreenLineProcessor = std::function<void(const Utility::ScreenDataPage&, const uint8_t)>;

		std::vector<ScreenLineProcessor> line_processors
		{
			{ std::bind(&OneTouchDevice::StatusProcessor_FilterPump, this, std::placeholders::_1, std::placeholders::_2) },
			{ std::bind(&OneTouchDevice::StatusProcessor_SolarHeat, this, std::placeholders::_1, std::placeholders::_2) },
			{ std::bind(&OneTouchDevice::StatusProcessor_PoolHeat, this, std::placeholders::_1, std::placeholders::_2) },
			{ std::bind(&OneTouchDevice::StatusProcessor_SpaHeat, this, std::placeholders::_1, std::placeholders::_2) },
			{ std::bind(&OneTouchDevice::StatusProcessor_AquaPurePercentage, this, std::placeholders::_1, std::placeholders::_2) },
			{ std::bind(&OneTouchDevice::StatusProcessor_SaltLevelPPM, this, std::placeholders::_1, std::placeholders::_2) },
			{ std::bind(&OneTouchDevice::StatusProcessor_CheckAquaPure, this, std::placeholders::_1, std::placeholders::_2) }
		};

		std::for_each(std::execution::par, line_processors.begin(), line_processors.end(), [&page](const auto& matcher_processor)
			{
				bool regex_matched_line = false;
				bool device_matched_line = false;

				// Ignore line 0 on every page as it's the "Equipment Status" title line...

				for (uint8_t line_id = 1; line_id < page.Size(); ++line_id)
				{
					if (page[line_id].Text.empty())
					{
						// Ignore empty lines...
					}
					else
					{
						matcher_processor(page, line_id);
					}
				}
			}
		);
	}

	void OneTouchDevice::PageProcessor_SelectSpeed(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_SelectSpeed", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_SelectSpeed page.");
	}

	void OneTouchDevice::PageProcessor_MenuHelp(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_MenuHelp", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_MenuHelp page.");
	}

	void OneTouchDevice::PageProcessor_SetTemperature(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_SetTemperature", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_SetTemperature page.");

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

		if (auto temperature = Utility::Temperature(Utility::TrimWhitespace(page[2].Text)); temperature().has_value())
		{
			JandyController::m_Config.PoolTemp(temperature().value());
		}

		if (auto temperature = Utility::Temperature(Utility::TrimWhitespace(page[3].Text)); temperature().has_value())
		{
			JandyController::m_Config.SpaTemp(temperature().value());
		}

		auto is_maintained = Utility::TrimWhitespace(page[5].Text);
		auto maintenance_hours = Utility::TrimWhitespace(page[6].Text);
	}

	void OneTouchDevice::PageProcessor_SetTime(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_SetTime", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_SetTime page.");
	}

	void OneTouchDevice::PageProcessor_SystemSetup(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_SystemSetup", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_SystemSetup page.");
	}

	void OneTouchDevice::PageProcessor_FreezeProtect(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_FreezeProtect", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_FreezeProtect page.");

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

		if (auto temperature = Utility::Temperature(Utility::TrimWhitespace(page[3].Text)); temperature().has_value())
		{
			JandyController::m_Config.FreezeProtectPoint(temperature().value());
		}
	}

	void OneTouchDevice::PageProcessor_Boost(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_Boost", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_Boost page.");
	}

	void OneTouchDevice::PageProcessor_SetAquapure(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_SetAquapure", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_SetAquapure page.");
	}

	void OneTouchDevice::PageProcessor_Version(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_Version", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_Version page.");

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
		
		const auto model_number = Utility::TrimWhitespace(page[4].Text);
		const auto panel_type = Utility::TrimWhitespace(page[5].Text);
		const auto fw_revision = Utility::TrimWhitespace(page[7].Text);

		Utility::PoolConfigurationDecoder pool_config_decoder(panel_type);

		JandyController::m_Config.PoolConfiguration = pool_config_decoder.Configuration();
		JandyController::m_Config.SystemBoard = pool_config_decoder.SystemBoard();
		JandyController::m_Config.EquipmentVersions.ModelNumber = model_number;
		JandyController::m_Config.EquipmentVersions.FirmwareRevision = fw_revision;
		
		LogInfo(Channel::Devices, std::format("Aqualink Power Center - Model: {}, Type: {}, Rev: {}", model_number, panel_type, fw_revision));
	}

	void OneTouchDevice::PageProcessor_DiagnosticsSensors(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_DiagnosticsSensors", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_DiagnosticsSensors page.");

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
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_DiagnosticsRemotes", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_DiagnosticsRemotes page.");

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
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_DiagnosticsErrors", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_DiagnosticsErrors page.");

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
