#include <algorithm>
#include <format>
#include <ranges>
#include <vector>

#include <boost/program_options.hpp>
#include <magic_enum/magic_enum.hpp>

#include "devices/jandy_device_id.h"
#include "formatters/jandy_device_formatters.h"
#include "logging/logging.h"
#include "options/options_jandy.h"
#include "options/helpers/conflicting_options_helper.h"
#include "options/helpers/option_dependency_helper.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Jandy::Options
{

	namespace
	{
		// Returns a default device ID for each emulated device type.
		// Uses secondary IDs to avoid conflicting with real (primary) devices on the RS-485 bus.
		JandyDeviceId DefaultDeviceId(JandyEmulatedDeviceTypes device_type)
		{
			switch (device_type)
			{
			case JandyEmulatedDeviceTypes::OneTouch:		return JandyDeviceId{ 0x41 };
			case JandyEmulatedDeviceTypes::IAQ:				return JandyDeviceId{ 0xA1 };
			case JandyEmulatedDeviceTypes::RS_Keypad:		return JandyDeviceId{ 0x09 };
			case JandyEmulatedDeviceTypes::PDA:				return JandyDeviceId{ 0x61 };
			case JandyEmulatedDeviceTypes::SerialAdapter:	return JandyDeviceId{ 0x48 };
			default:										return JandyDeviceId{ 0xFF };
			}
		}
	}
	// anonymous namespace

	boost::program_options::options_description OptionsProcessor::Options() const
	{
		boost::program_options::options_description options(SettingsType::AreaName(), Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", JandyOptionsCollection.size(), SettingsType::AreaName()));
		for (auto& option : JandyOptionsCollection)
		{
			options.add((*option)());
		}

		return options;
	}

	void OptionsProcessor::Validate(const boost::program_options::variables_map& vm) const
	{
		// Device IDs require device types, but device types can stand alone (defaults will be assigned).
		Helper_ValidateOptionDependencies(vm, OPTION_EMULATEDDEVICEID, OPTION_EMULATEDDEVICETYPE);

		Helper_CheckForConflictingOptions(vm, OPTION_DISABLEEMULATION, OPTION_EMULATEDDEVICETYPE);
		Helper_CheckForConflictingOptions(vm, OPTION_DISABLEEMULATION, OPTION_EMULATEDDEVICEID);
	}

	std::expected<OptionsProcessor::SettingsType, ErrorCodes::Options_ErrorCodes> OptionsProcessor::Process(boost::program_options::variables_map& vm) const
	{
		SettingsType settings;

		if (OPTION_DISABLEEMULATION->IsPresent(vm))
		{
			settings.disable_emulation = OPTION_DISABLEEMULATION->As<bool>(vm);
		}

		if (OPTION_NAVPASSWORD->IsPresent(vm))
		{
			settings.navigation_password = OPTION_NAVPASSWORD->As<std::string>(vm);
			if (!settings.navigation_password.empty())
			{
				LogInfo(Channel::Options, "Navigation password configured for menu access");
			}
		}

		if (!OPTION_EMULATEDDEVICETYPE->IsPresent(vm))
		{
			// No explicit device types specified — use default set.
			// OneTouch for menu scraping, IAQ for status data, SerialAdapter for commands.
			static const std::vector<JandyEmulatedDeviceTypes> DEFAULT_DEVICE_TYPES
			{
				JandyEmulatedDeviceTypes::OneTouch,
				JandyEmulatedDeviceTypes::IAQ,
				JandyEmulatedDeviceTypes::SerialAdapter
			};

			for (auto type : DEFAULT_DEVICE_TYPES)
			{
				settings.emulated_devices.emplace_back(type, DefaultDeviceId(type));
			}

			LogInfo(Channel::Options, "No emulated device types specified; using defaults: OneTouch, IAQ, SerialAdapter");
		}
		else
		{
			// Device types and IDs are parsed and validated by boost::program_options via the
			// type-specific validators (multitoken, space-separated values on the command line).
			auto device_types = OPTION_EMULATEDDEVICETYPE->As<std::vector<JandyEmulatedDeviceTypes>>(vm);
			std::vector<Devices::JandyDeviceId> device_ids;

			if (OPTION_EMULATEDDEVICEID->IsPresent(vm))
			{
				device_ids = OPTION_EMULATEDDEVICEID->As<std::vector<Devices::JandyDeviceId>>(vm);
			}

			// Assign default device IDs for any types that don't have an explicit ID.
			// Defaults use a secondary ID to avoid conflicting with real devices on the bus.
			while (device_ids.size() < device_types.size())
			{
				auto index = device_ids.size();
				auto default_id = DefaultDeviceId(device_types[index]);

				LogDebug(Channel::Options, std::format("Assigning default device id 0x{:02x} for emulated device type: {}",
					default_id(), magic_enum::enum_name(device_types[index])));

				device_ids.push_back(default_id);
			}

			std::ranges::transform(
				std::views::zip(device_types, device_ids),
				std::back_inserter(settings.emulated_devices),
				[](auto pair)
				{
					auto& [type, id] = pair;
					LogDebug(Channel::Options, std::format("Emplacing emulated device: {} (id: {})", magic_enum::enum_name(type), id));
					return JandyEmulatedDevice(type, id);
				}
			);
		}

		return settings;
	}

}
// namespace AqualinkAutomate::Jandy::Options
