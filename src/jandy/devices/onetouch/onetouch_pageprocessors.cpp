#include <execution>
#include <functional>
#include <format>
#include <memory>
#include <tuple>

#include <re2/re2.h>

#include "logging/logging.h"
#include "jandy/auxillaries/jandy_auxillary_traits_types.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/factories/jandy_auxillary_factory.h"
#include "jandy/utility/jandy_pool_configuration_decoder.h"
#include "jandy/utility/string_manipulation.h"
#include "jandy/utility/string_conversion/auxillary_state_string_converter.h"
#include "jandy/utility/string_conversion/temperature_string_converter.h"

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

		JandyController::m_DataHub->Mode = Equipment::JandyEquipmentModes::Normal;

		///FIXME - get pool / spa temperature if FP pump is running....

		if (auto temperature = Utility::TemperatureStringConverter(Utility::TrimWhitespace(page[6].Text)); temperature().has_value())
		{
			JandyController::m_DataHub->AirTemp(temperature().value());
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

		JandyController::m_DataHub->Mode = Equipment::JandyEquipmentModes::Service;
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

		JandyController::m_DataHub->Mode = Equipment::JandyEquipmentModes::TimeOut;
		JandyController::m_DataHub->TimeoutRemaining = Utility::TimeoutDurationStringConverter(Utility::TrimWhitespace(page[10].Text));
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
			auto new_aux_state = Utility::AuxillaryStateStringConverter(Utility::TrimWhitespace(page[row_index].Text));
			
			if (auto aux_ptr = Factory::JandyAuxillaryFactory::Instance().OneTouchDevice_CreateDevice(new_aux_state); !aux_ptr.has_value())
			{
				LogTrace(Channel::Devices, std::format("Failed to create a device for this specific devic's row text: {}", Utility::TrimWhitespace(page[row_index].Text)));
			}
			else
			{
				JandyController::m_DataHub->Devices.Add(aux_ptr.value());
			}
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

		if (auto temperature = Utility::TemperatureStringConverter(Utility::TrimWhitespace(page[2].Text)); temperature().has_value())
		{
			JandyController::m_DataHub->PoolTemp(temperature().value());
		}

		if (auto temperature = Utility::TemperatureStringConverter(Utility::TrimWhitespace(page[3].Text)); temperature().has_value())
		{
			JandyController::m_DataHub->SpaTemp(temperature().value());
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

		if (auto temperature = Utility::TemperatureStringConverter(Utility::TrimWhitespace(page[3].Text)); temperature().has_value())
		{
			JandyController::m_DataHub->FreezeProtectPoint(temperature().value());
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

		JandyController::m_DataHub->PoolConfiguration = pool_config_decoder.Configuration();
		JandyController::m_DataHub->SystemBoard = pool_config_decoder.SystemBoard();
		JandyController::m_DataHub->EquipmentVersions.ModelNumber = model_number;
		JandyController::m_DataHub->EquipmentVersions.FirmwareRevision = fw_revision;
		
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

	void OneTouchDevice::PageProcessor_LabelAuxList(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_LabelAuxList", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_LabelAuxList page.");

		/*
			Info:   OneTouch Menu Line 00 =    Label Aux
			Info:   OneTouch Menu Line 01 =
			Info:   OneTouch Menu Line 02 = Aux1           >
			Info:   OneTouch Menu Line 03 = Aux2           >
			Info:   OneTouch Menu Line 04 = Aux3           >
			Info:   OneTouch Menu Line 05 = Aux4           >
			Info:   OneTouch Menu Line 06 = Aux5           >
			Info:   OneTouch Menu Line 07 = Aux6           >
			Info:   OneTouch Menu Line 08 = Aux7           >
			Info:   OneTouch Menu Line 09 = Aux B1         >
			Info:   OneTouch Menu Line 10 =    ^^ More vv
			Info:   OneTouch Menu Line 11 =           
		*/
	}

	void OneTouchDevice::PageProcessor_LabelAux(const Utility::ScreenDataPage& page)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("PageProcessor_LabelAux", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is processing a PageProcessor_LabelAux page.");

		/*
			Info:   OneTouch Menu Line 00 =    Label Aux1
			Info:   OneTouch Menu Line 01 =
			Info:   OneTouch Menu Line 02 =  Current Label 
			Info:   OneTouch Menu Line 03 =       Aux1      
			Info:   OneTouch Menu Line 04 =                
			Info:   OneTouch Menu Line 05 = General Labels >
			Info:   OneTouch Menu Line 06 = Light   Labels >
			Info:   OneTouch Menu Line 07 = Wtrfall Labels >
			Info:   OneTouch Menu Line 08 = Custom  Label  >
			Info:   OneTouch Menu Line 09 =
			Info:   OneTouch Menu Line 10 =
			Info:   OneTouch Menu Line 11 =           
		*/

		const auto aux_custom_label = Utility::TrimWhitespace(page[3].Text);

		re2::RE2 pattern("(Label) (Aux(\\s?[B-D][1-8]|\\s?[1-7]))");
		std::string label, aux_id_string;

		if (aux_custom_label.empty())
		{
			LogDebug(Channel::Devices, "Custom auxillary label was not set; cannot continue");
		}
		else if (!RE2::FullMatch(Utility::TrimWhitespace(page[0].Text), pattern, &label, &aux_id_string))
		{
			LogDebug(Channel::Devices, std::format("Failed to parse the row text looking for an auxillary id; text was {}", Utility::TrimWhitespace(page[0].Text)));
		}
		else if (auto aux_id = magic_enum::enum_cast<Auxillaries::JandyAuxillaryIds>(aux_id_string); !aux_id.has_value())
		{
			LogDebug(Channel::Devices, std::format("Failed to generate the id for Auxillary Device given string {}", aux_id_string));
		}
		else 
		{
			std::shared_ptr<Kernel::AuxillaryDevice> aux_ptr(nullptr);

			if (auto aux_collection = m_DataHub->Devices.FindByTrait(Auxillaries::JandyAuxillaryId{}, aux_id.value()); aux_collection.empty())
			{
				if (auto temp_ptr = Factory::JandyAuxillaryFactory::Instance().SerialAdapterDevice_CreateDevice(aux_id.value()); temp_ptr.has_value())
				{
					LogDebug(Channel::Devices, std::format("Failed to create a new Auxillary Device for aux id: {}", magic_enum::enum_name(aux_id.value())));
				}
				else
				{
					aux_ptr = temp_ptr.value();
				}
			}
			else if (1 < aux_collection.size())
			{
				LogDebug(Channel::Devices, std::format("Found {} instances of Auxillary Device with aux id: {}; cannot attach custom label", aux_collection.size(), magic_enum::enum_name(aux_id.value())));
			}
			else
			{
				aux_ptr = aux_collection.front();
			}

			if (nullptr == aux_ptr)
			{
				LogDebug(Channel::Devices, std::format("Failed to find the Auxillary Device for aux id: {}; cannot set custom label", magic_enum::enum_name(aux_id.value())));
			}
			else
			{
				aux_ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, aux_custom_label);
			}
		}
	}

}
// namespace AqualinkAutomate::Devices
