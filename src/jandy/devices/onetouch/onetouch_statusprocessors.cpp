#include <array>
#include <cctype>

#include <boost/regex.hpp>

#include "devices/onetouch_device.h"
#include "kernel/auxillary_traits/auxillary_traits_helpers.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/hub_events/data_hub_config_event_button_state_change.h"
#include "logging/logging.h"
#include "utility/string_manipulation.h"
#include "utility/to_number.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	bool OneTouchDevice::StatusProcessor_ShouldSkipLineProcessing(const HintArrayType& hint_array, const std::string_view line_to_process) const
	{
		// ASSUMPTIONS:
		//
		//   - The line of text referenced will always be in lowercase
		//   - The format of text on the line eg. SALT XXXX PPM is fixed
		//   - Hints are contiguous and the first two characters
		//

		using line_size_type = decltype(line_to_process)::size_type;
		line_size_type first_char_position = 0;
		bool skip_line_processing = true;

		for (auto ch : line_to_process)
		{
			if (line_to_process.size() <= first_char_position || hint_array[0] != std::tolower(static_cast<unsigned char>(ch)))
			{
				// At the end of the line or first hint didn't match...don't bother checking the rest
				break;
			}

			if (line_to_process.size() > (first_char_position + 1) && hint_array[1] == std::tolower(static_cast<unsigned char>(line_to_process[first_char_position + 1])))
			{
				// It is worth processing the entire line.
				skip_line_processing = false;
			}

			// No matter the outcome, there's no need to check the rest of the line.
			break;

			// Increment the index of the character we're looking at
			++first_char_position;
		}

		return skip_line_processing;
	}

	void OneTouchDevice::StatusProcessor_FilterPump(const Utility::ScreenDataPage& page, const uint8_t line_id)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::StatusProcessor_FilterPump", std::source_location::current());

		LogDebug(Channel::Devices, std::format("OneTouch ({}): OneTouch device is checking for a StatusProcessor_FilterPump status line.", DeviceId()));

		static const boost::regex re("filter pump", boost::regex_constants::icase);
		static const HintArrayType hints{ 'f', 'i' };

		if (const auto line_to_process = Utility::TrimWhitespace(page[line_id].Text); StatusProcessor_ShouldSkipLineProcessing(hints, line_to_process))
		{
			LogTrace(Channel::Devices, std::format("OneTouch ({}): StatusProcessor_FilterPump skipping line processing; hints were not matched", DeviceId()));
		} 
		else if (!boost::regex_match(line_to_process, re))
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Failed while processing StatusProcessor_FilterPump status line; failed to identify pump line", DeviceId()));
		}
		else
		{
			if (0 == m_DataHub->FilterPumps().size())
			{
				// No primary filter pump...add it
				auto ptr = std::make_shared<Kernel::AuxillaryDevice>();
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{}, Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Pump);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, Kernel::AuxillaryTraitsTypes::LabelTrait::COMMON_LABEL_FILTER_PUMP);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::PumpSpeedTrait{}, Kernel::PumpSpeeds::Unknown);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::PumpStatusTrait{}, Kernel::PumpStatuses::Unknown);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::PumpTypeTrait{}, Kernel::PumpTypes::FilterCirculation);
				m_DataHub->Devices.Add(std::move(ptr));
			}

			for (auto& pump : m_DataHub->FilterPumps())
			{
				// AquaLink RS Equipment Status shows a single "Filter Pump" line;
				// individual pump identification requires Intelliflo-specific messages.
				LogTrace(Channel::Devices, std::format("OneTouch ({}): StatusProcessor_FilterPump setting filter pump status trait to '{}'", DeviceId(), magic_enum::enum_name(Kernel::PumpStatuses::Running)));
				pump->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::PumpStatusTrait{}, Kernel::PumpStatuses::Running);

				// Signal that a button state change has occurred.
				auto status_string = Kernel::AuxillaryTraitsTypes::ConvertStatusToString(pump);
				auto update_event = std::make_shared<Kernel::DataHub_ConfigEvent_ButtonStateChange>(pump->Id(), status_string);
				m_DataHub->ConfigUpdateSignal(update_event);
			}
		}
	}

	void OneTouchDevice::StatusProcessor_PoolHeat(const Utility::ScreenDataPage& page, const uint8_t line_id)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::StatusProcessor_PoolHeat", std::source_location::current());

		LogDebug(Channel::Devices, std::format("OneTouch ({}): OneTouch device is checking for a StatusProcessor_PoolHeat status line.", DeviceId()));

		static const boost::regex re("(pool heat)(?:\\s+(ena))?", boost::regex_constants::icase);
		static const HintArrayType hints{ 'p', 'o' };
		boost::smatch matches;

		if (const auto line_to_process = Utility::TrimWhitespace(page[line_id].Text); StatusProcessor_ShouldSkipLineProcessing(hints, line_to_process))
		{
			LogTrace(Channel::Devices, std::format("OneTouch ({}): StatusProcessor_PoolHeat skipping line processing; hints were not matched", DeviceId()));
		}
		else if (!boost::regex_match(line_to_process, matches, re))
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Failed while processing StatusProcessor_PoolHeat status line; failed to identify heat line", DeviceId()));
		}
		else if (false == matches[1].matched)
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Failed while processing StatusProcessor_PoolHeat status line; incorrect token count returned", DeviceId()));
		}
		else
		{
			// Match for at least POOL HEAT
			using Kernel::AuxillaryTraitsTypes::HeaterStatusTrait;

			const std::string pool_heater_label{ "Pool Heat" };

			if (0 == m_DataHub->Devices.FindByLabel(pool_heater_label).size())
			{
				// Check for installed pool heating.  If it doesn't exist, add it.
				auto ptr = std::make_shared<Kernel::AuxillaryDevice>();
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{}, Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Heater);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, pool_heater_label);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::HeaterStatusTrait{}, Kernel::HeaterStatuses::Off);
				m_DataHub->Devices.Add(std::move(ptr));
			}

			// The status is either going to be 'Heating' because only POOL HEAT was matched or 'Enabled' because POOL HEAT ENA was matched
			const auto heater_status = (false == matches[2].matched) ? Kernel::HeaterStatuses::Heating : Kernel::HeaterStatuses::Enabled;

			LogTrace(Channel::Devices, std::format("OneTouch ({}): StatusProcessor_PoolHeat setting Pool Heating status trait to '{}'", DeviceId(), magic_enum::enum_name(heater_status)));
			auto heater = m_DataHub->Devices.FindByLabel(pool_heater_label).front();
			heater->AuxillaryTraits.Set(HeaterStatusTrait{}, heater_status);

			// Signal that a button state change has occurred.
			auto status_string = Kernel::AuxillaryTraitsTypes::ConvertStatusToString(heater);
			auto update_event = std::make_shared<Kernel::DataHub_ConfigEvent_ButtonStateChange>(heater->Id(), status_string);
			m_DataHub->ConfigUpdateSignal(update_event);
		}
	}

	void OneTouchDevice::StatusProcessor_SpaHeat(const Utility::ScreenDataPage& page, const uint8_t line_id)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::StatusProcessor_SpaHeat", std::source_location::current());

		LogDebug(Channel::Devices, std::format("OneTouch ({}): OneTouch device is checking for a StatusProcessor_SpaHeat status line.", DeviceId()));

		static const boost::regex re("(spa heat)(?:\\s+(ena))?", boost::regex_constants::icase);
		static const HintArrayType hints{ 's', 'p' };
		boost::smatch matches;

		if (const auto line_to_process = Utility::TrimWhitespace(page[line_id].Text); StatusProcessor_ShouldSkipLineProcessing(hints, line_to_process))
		{
			LogTrace(Channel::Devices, std::format("OneTouch ({}): StatusProcessor_SpaHeat skipping line processing; hints were not matched", DeviceId()));
		}
		else if (!boost::regex_match(line_to_process, matches, re))
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Failed while processing StatusProcessor_SpaHeat status line; failed to identify heat line", DeviceId()));
		}
		else if (false == matches[1].matched)
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Failed while processing StatusProcessor_SpaHeat status line; incorrect token count returned", DeviceId()));
		}
		else
		{
			// Match for at least SPA HEAT
			using Kernel::AuxillaryTraitsTypes::HeaterStatusTrait;

			const std::string spa_heater_label{ "Spa Heat" };

			if (0 == m_DataHub->Devices.FindByLabel(spa_heater_label).size())
			{
				// Check for installed solar heating.  If it doesn't exist, add it.
				auto ptr = std::make_shared<Kernel::AuxillaryDevice>();
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{}, Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Heater);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, spa_heater_label);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::HeaterStatusTrait{}, Kernel::HeaterStatuses::Off);
				m_DataHub->Devices.Add(std::move(ptr));
			}

			// The status is either going to be 'Heating' because only SPA HEAT was matched or 'Enabled' because SPA HEAT ENA was matched
			const auto heater_status = (false == matches[2].matched) ? Kernel::HeaterStatuses::Heating : Kernel::HeaterStatuses::Enabled;

			LogTrace(Channel::Devices, std::format("OneTouch ({}): StatusProcessor_SpaHeat setting Spa Heating status trait to '{}'", DeviceId(), magic_enum::enum_name(heater_status)));
			auto heater = m_DataHub->Devices.FindByLabel(spa_heater_label).front();
			heater->AuxillaryTraits.Set(HeaterStatusTrait{}, heater_status);

			// Signal that a button state change has occurred.
			auto status_string = Kernel::AuxillaryTraitsTypes::ConvertStatusToString(heater);
			auto update_event = std::make_shared<Kernel::DataHub_ConfigEvent_ButtonStateChange>(heater->Id(), status_string);
			m_DataHub->ConfigUpdateSignal(update_event);
		}
	}

	void OneTouchDevice::StatusProcessor_SolarHeat(const Utility::ScreenDataPage& page, const uint8_t line_id)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::StatusProcessor_SolarHeat", std::source_location::current());

		LogDebug(Channel::Devices, std::format("OneTouch ({}): OneTouch device is checking for a StatusProcessor_SolarHeat status line.", DeviceId()));

		static const boost::regex re("(solar heat)(?:\\s+(ena))?", boost::regex_constants::icase);
		static const HintArrayType hints{ 's', 'o' };
		boost::smatch matches;

		if (const auto line_to_process = Utility::TrimWhitespace(page[line_id].Text); StatusProcessor_ShouldSkipLineProcessing(hints, line_to_process))
		{
			LogTrace(Channel::Devices, std::format("OneTouch ({}): StatusProcessor_SolarHeat skipping line processing; hints were not matched", DeviceId()));
		}
		else if (!boost::regex_match(line_to_process, matches, re))
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Failed while processing StatusProcessor_SolarHeat status line; failed to identify heat line", DeviceId()));
		}
		else if (false == matches[1].matched)
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Failed while processing StatusProcessor_SolarHeat status line; incorrect token count returned", DeviceId()));
		}
		else 
		{
			// Match for at least SOLAR HEAT
			using Kernel::AuxillaryTraitsTypes::HeaterStatusTrait;

			const std::string solar_heater_label{ "Solar Heat" };

			if (0 == m_DataHub->Devices.FindByLabel(solar_heater_label).size())
			{
				// Check for installed solar heating.  If it doesn't exist, add it.
				auto ptr = std::make_shared<Kernel::AuxillaryDevice>();
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{}, Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Heater);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, solar_heater_label);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::HeaterStatusTrait{}, Kernel::HeaterStatuses::Off);
				m_DataHub->Devices.Add(std::move(ptr));
			}

			// The status is either going to be 'Heating' because only SOLAR HEAT was matched or 'Enabled' because SOLAR HEAT ENA was matched
			const auto heater_status = (false == matches[2].matched) ? Kernel::HeaterStatuses::Heating : Kernel::HeaterStatuses::Enabled;

			LogTrace(Channel::Devices, std::format("OneTouch ({}): StatusProcessor_SolarHeat setting Solar Heating status trait to '{}'", DeviceId(), magic_enum::enum_name(heater_status)));
			auto heater = m_DataHub->Devices.FindByLabel(solar_heater_label).front();
			heater->AuxillaryTraits.Set(HeaterStatusTrait{}, heater_status);

			// Signal that a button state change has occurred.
			auto status_string = Kernel::AuxillaryTraitsTypes::ConvertStatusToString(heater);
			auto update_event = std::make_shared<Kernel::DataHub_ConfigEvent_ButtonStateChange>(heater->Id(), status_string);
			m_DataHub->ConfigUpdateSignal(update_event);
		}
	}

	void OneTouchDevice::StatusProcessor_HeatPump(const Utility::ScreenDataPage& page, const uint8_t line_id)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::StatusProcessor_HeatPump", std::source_location::current());

		LogDebug(Channel::Devices, std::format("OneTouch ({}): OneTouch device is checking for a StatusProcessor_HeatPump status line.", DeviceId()));

		static const boost::regex re("(heat pump)(?:\\s+(ena))?", boost::regex_constants::icase);
		static const HintArrayType hints{ 'h', 'e' };
		boost::smatch matches;

		if (const auto line_to_process = Utility::TrimWhitespace(page[line_id].Text); StatusProcessor_ShouldSkipLineProcessing(hints, line_to_process))
		{
			LogTrace(Channel::Devices, std::format("OneTouch ({}): StatusProcessor_HeatPump skipping line processing; hints were not matched", DeviceId()));
		}
		else if (!boost::regex_match(line_to_process, matches, re))
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Failed while processing StatusProcessor_HeatPump status line; failed to identify heat line", DeviceId()));
		}
		else if (false == matches[1].matched)
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Failed while processing StatusProcessor_HeatPump status line; incorrect token count returned", DeviceId()));
		}
		else
		{
			// Match for at least HEAT PUMP
			using Kernel::AuxillaryTraitsTypes::HeaterStatusTrait;

			const std::string heat_pump_label{ "Heat Pump" };

			if (0 == m_DataHub->Devices.FindByLabel(heat_pump_label).size())
			{
				// Check for installed heat pump heating.  If it doesn't exist, add it.
				auto ptr = std::make_shared<Kernel::AuxillaryDevice>();
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{}, Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Heater);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, heat_pump_label);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::HeaterStatusTrait{}, Kernel::HeaterStatuses::Off);
				m_DataHub->Devices.Add(std::move(ptr));
			}

			// The status is either going to be 'Heating' because only HEAT PUMP was matched or 'Enabled' because HEAT PUMP ENA was matched
			const auto heater_status = (false == matches[2].matched) ? Kernel::HeaterStatuses::Heating : Kernel::HeaterStatuses::Enabled;

			LogTrace(Channel::Devices, std::format("OneTouch ({}): StatusProcessor_HeatPump setting Heat Pump Heating status trait to '{}'", DeviceId(), magic_enum::enum_name(heater_status)));
			auto heater = m_DataHub->Devices.FindByLabel(heat_pump_label).front();
			heater->AuxillaryTraits.Set(HeaterStatusTrait{}, heater_status);

			// Signal that a button state change has occurred.
			auto status_string = Kernel::AuxillaryTraitsTypes::ConvertStatusToString(heater);
			auto update_event = std::make_shared<Kernel::DataHub_ConfigEvent_ButtonStateChange>(heater->Id(), status_string);
			m_DataHub->ConfigUpdateSignal(update_event);
		}
	}

	void OneTouchDevice::StatusProcessor_Chiller(const Utility::ScreenDataPage& page, const uint8_t line_id)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::StatusProcessor_Chiller", std::source_location::current());

		LogDebug(Channel::Devices, std::format("OneTouch ({}): OneTouch device is checking for a StatusProcessor_Chiller status line.", DeviceId()));

		static const boost::regex re("(chiller)(?:\\s+(ena))?", boost::regex_constants::icase);
		static const HintArrayType hints{ 'c', 'h' };
		boost::smatch matches;

		if (const auto line_to_process = Utility::TrimWhitespace(page[line_id].Text); StatusProcessor_ShouldSkipLineProcessing(hints, line_to_process))
		{
			LogTrace(Channel::Devices, std::format("OneTouch ({}): StatusProcessor_Chiller skipping line processing; hints were not matched", DeviceId()));
		}
		else if (!boost::regex_match(line_to_process, matches, re))
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Failed while processing StatusProcessor_Chiller status line; failed to identify chiller line", DeviceId()));
		}
		else if (false == matches[1].matched)
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Failed while processing StatusProcessor_Chiller status line; incorrect token count returned", DeviceId()));
		}
		else
		{
			// Match for at least CHILLER
			using Kernel::AuxillaryTraitsTypes::HeaterStatusTrait;

			const std::string chiller_label{ "ChillerCooling" };

			if (0 == m_DataHub->Devices.FindByLabel(chiller_label).size())
			{
				// Check for installed chiller cooling.  If it doesn't exist, add it.
				auto ptr = std::make_shared<Kernel::AuxillaryDevice>();
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{}, Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Heater);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, chiller_label);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::HeaterStatusTrait{}, Kernel::HeaterStatuses::Off);
				m_DataHub->Devices.Add(std::move(ptr));
			}

			// The status is either going to be 'Heating' because only CHILLER was matched or 'Enabled' because CHILLER ENA was matched
			const auto heater_status = (false == matches[2].matched) ? Kernel::HeaterStatuses::Heating : Kernel::HeaterStatuses::Enabled;

			LogTrace(Channel::Devices, std::format("OneTouch ({}): StatusProcessor_Chiller setting Chiller Cooling status trait to '{}'", DeviceId(), magic_enum::enum_name(heater_status)));
			auto chiller = m_DataHub->Devices.FindByLabel(chiller_label).front();
			chiller->AuxillaryTraits.Set(HeaterStatusTrait{}, heater_status);

			// Signal that a button state change has occurred.
			auto status_string = Kernel::AuxillaryTraitsTypes::ConvertStatusToString(chiller);
			auto update_event = std::make_shared<Kernel::DataHub_ConfigEvent_ButtonStateChange>(chiller->Id(), status_string);
			m_DataHub->ConfigUpdateSignal(update_event);
		}
	}

	void OneTouchDevice::StatusProcessor_AquaPurePercentage(const Utility::ScreenDataPage& page, const uint8_t line_id)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::StatusProcessor_AquaPurePercentage", std::source_location::current());

		LogDebug(Channel::Devices, std::format("OneTouch ({}): OneTouch device is checking for a StatusProcessor_AquaPurePercentage status line.", DeviceId()));

		static const boost::regex re("aquapure ([0-9]{1,2}|100)%", boost::regex_constants::icase);
		static const HintArrayType hints{ 'a', 'q' };
		boost::smatch matches;

		if (const auto line_to_process = Utility::TrimWhitespace(page[line_id].Text); StatusProcessor_ShouldSkipLineProcessing(hints, line_to_process))
		{
			LogTrace(Channel::Devices, std::format("OneTouch ({}): StatusProcessor_AquaPurePercentage skipping line processing; hints were not matched", DeviceId()));
		}
		else if (!boost::regex_match(line_to_process, matches, re))
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Failed while processing StatusProcessor_AquaPurePercentage status line; failed to identify percentage", DeviceId()));
		}
		else if (false == matches[1].matched)
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Failed while processing StatusProcessor_AquaPurePercentage status line; incorrect token count returned", DeviceId()));
		}
		else if (const auto percentage_dutycycle_string = matches[1].str(); percentage_dutycycle_string.empty())
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Failed while processing StatusProcessor_AquaPurePercentage status line; sub-match is empty", DeviceId()));
		}
		else if (const auto percentage_dutycycle = Utility::ToNumber<uint8_t>(percentage_dutycycle_string); !percentage_dutycycle.has_value())
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Percentage string-to-number conversation failed: original value was '{}'", DeviceId(), percentage_dutycycle_string));
		}
		else 
		{
			using Kernel::AuxillaryTraitsTypes::DutyCycleTrait;

			const std::string chlorinator_label{"AquaPure"};

			if (0 == m_DataHub->Devices.FindByLabel(chlorinator_label).size())
			{
				// Check for an installed chlorinator.  If one doesn't exist, add one.
				auto ptr = std::make_shared<Kernel::AuxillaryDevice>();
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{}, Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Chlorinator);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, chlorinator_label);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::ChlorinatorStatusTrait{}, Kernel::ChlorinatorStatuses::Running);
				m_DataHub->Devices.Add(std::move(ptr));
			}

			m_DataHub->Devices.FindByLabel(chlorinator_label).front()->AuxillaryTraits.Set(DutyCycleTrait{}, percentage_dutycycle.value());
		}
	}

	void OneTouchDevice::StatusProcessor_SaltLevelPPM(const Utility::ScreenDataPage& page, const uint8_t line_id)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::StatusProcessor_SaltLevelPPM", std::source_location::current());
		LogDebug(Channel::Devices, std::format("OneTouch ({}): OneTouch device is checking for a StatusProcessor_SaltLevelPPM status line.", DeviceId()));

		static const boost::regex re("salt ([0-9]{1,4}) ppm", boost::regex_constants::icase);
		static const HintArrayType hints{ 's', 'a' };
		boost::smatch matches;
		
		if (const auto line_to_process = Utility::TrimWhitespace(page[line_id].Text); StatusProcessor_ShouldSkipLineProcessing(hints, line_to_process))
		{
			LogTrace(Channel::Devices, std::format("OneTouch ({}): StatusProcessor_SaltLevelPPM skipping line processing; hints were not matched", DeviceId()));
		} 
		else if (!boost::regex_match(line_to_process, matches, re))
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Failed while processing StatusProcessor_SaltLevelPPM status line; failed to identify salt level", DeviceId()));
		}
		else if (false == matches[1].matched)
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Failed while processing StatusProcessor_SaltLevelPPM status line; incorrect token count returned", DeviceId()));
		}
		else if (const auto salt_level_string = matches[1].str(); salt_level_string.empty())
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Failed while processing StatusProcessor_SaltLevelPPM status line; sub-match is empty", DeviceId()));
		}
		else  if (const auto salt_level = Utility::ToNumber<uint32_t>(salt_level_string); !salt_level.has_value())
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Salt level string-to-number conversation failed: original value was '{}'", DeviceId(), salt_level_string));
		}
		else
		{
			m_DataHub->SaltLevel(salt_level.value() * ppm);
		}
	}

	void OneTouchDevice::StatusProcessor_CheckAquaPure(const Utility::ScreenDataPage& page, const uint8_t line_id)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("OneTouchDevice::StatusProcessor_CheckAquaPure", std::source_location::current());

		LogDebug(Channel::Devices, std::format("OneTouch ({}): OneTouch device is checking for a StatusProcessor_CheckAquaPure status line.", DeviceId()));

		static const boost::regex re("check aquapure", boost::regex_constants::icase);
		static const HintArrayType hints{ 'c', 'h' };

		if (const auto line_to_process = Utility::TrimWhitespace(page[line_id].Text); StatusProcessor_ShouldSkipLineProcessing(hints, line_to_process))
		{
			LogTrace(Channel::Devices, std::format("OneTouch ({}): StatusProcessor_CheckAquaPure skipping line processing; hints were not matched", DeviceId()));
		}
		else if (!boost::regex_match(line_to_process, re))
		{
			LogDebug(Channel::Devices, std::format("OneTouch ({}): Failed while processing StatusProcessor_CheckAquaPure status line; failed to identify check line", DeviceId()));
		}
		else
		{
			const std::string chlorinator_label{"AquaPure"};

			if (0 == m_DataHub->Devices.FindByLabel(chlorinator_label).size())
			{
				// Check for an installed chlorinator.  If one doesn't exist, add one.
				auto ptr = std::make_shared<Kernel::AuxillaryDevice>();
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{}, Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Chlorinator);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, chlorinator_label);
				ptr->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::ChlorinatorStatusTrait{}, Kernel::ChlorinatorStatuses::Running);
				m_DataHub->Devices.Add(std::move(ptr));
			}

			// "Check AquaPure" on the Equipment Status page is a binary alert;
			// specific error codes are decoded from AquaRite RS-485 messages.
			// Flag the chlorinator status so consumers know there is a problem.
			auto chlorinator = m_DataHub->Devices.FindByLabel(chlorinator_label).front();
			LogTrace(Channel::Devices, std::format("OneTouch ({}): StatusProcessor_CheckAquaPure setting chlorinator status to Unknown (check system alert)", DeviceId()));
			chlorinator->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::ChlorinatorStatusTrait{}, Kernel::ChlorinatorStatuses::Unknown);

			// Signal that a button state change has occurred.
			auto status_string = Kernel::AuxillaryTraitsTypes::ConvertStatusToString(chlorinator);
			auto update_event = std::make_shared<Kernel::DataHub_ConfigEvent_ButtonStateChange>(chlorinator->Id(), status_string);
			m_DataHub->ConfigUpdateSignal(update_event);
		}
	}

}
// namespace AqualinkAutomate::Devices
