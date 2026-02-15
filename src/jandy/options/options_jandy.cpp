#include <algorithm>
#include <charconv>
#include <cstdint>
#include <format>
#include <limits>
#include <ranges>
#include <sstream>

#include <boost/program_options.hpp>
#include <magic_enum/magic_enum.hpp>

#include "devices/jandy_device_id.h"
#include "formatters/jandy_device_formatters.h"
#include "logging/logging.h"
#include "options/options_jandy.h"
#include "options/helpers/conflicting_options_helper.h"
#include "options/helpers/option_dependency_helper.h"
#include "utility/case_insensitive_comparision.h"
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
			case JandyEmulatedDeviceTypes::SerialAdapter:	return JandyDeviceId{ 0x49 };
			default:										return JandyDeviceId{ 0xFF };
			}
		}

		std::vector<JandyEmulatedDeviceTypes> ParseDeviceTypes(const std::string& csv)
		{
			std::vector<JandyEmulatedDeviceTypes> result;
			std::istringstream stream(csv);
			std::string token;

			while (std::getline(stream, token, ','))
			{
				// Trim whitespace
				auto start = token.find_first_not_of(" \t");
				auto end = token.find_last_not_of(" \t");

				if (start == std::string::npos)
				{
					LogDebug(Channel::Options, "Invalid conversion of emulated device type: empty token in comma-separated list");
					throw boost::program_options::validation_error(boost::program_options::validation_error::invalid_option_value);
				}

				auto trimmed = token.substr(start, end - start + 1);

				if (auto enum_value = magic_enum::enum_cast<JandyEmulatedDeviceTypes>(trimmed, Utility::case_insensitive_comparision); enum_value.has_value())
				{
					result.push_back(enum_value.value());
				}
				else
				{
					LogDebug(Channel::Options, std::format("Invalid conversion of emulated device type -> provided string was: {}", trimmed));
					throw boost::program_options::validation_error(boost::program_options::validation_error::invalid_option_value);
				}
			}

			return result;
		}

		std::vector<JandyDeviceId> ParseDeviceIds(const std::string& csv)
		{
			std::vector<JandyDeviceId> result;
			std::istringstream stream(csv);
			std::string token;

			while (std::getline(stream, token, ','))
			{
				// Trim whitespace
				auto start = token.find_first_not_of(" \t");
				auto end = token.find_last_not_of(" \t");

				if (start == std::string::npos)
				{
					LogDebug(Channel::Options, "Invalid conversion of emulated device id: empty token in comma-separated list");
					throw boost::program_options::validation_error(boost::program_options::validation_error::invalid_option_value);
				}

				auto trimmed = token.substr(start, end - start + 1);

				if (trimmed.length() < 3 || trimmed.length() > 4 || std::tolower(trimmed[1]) != 'x')
				{
					LogDebug(Channel::Options, std::format("Invalid conversion of emulated device id: incorrectly formatted -> {}", trimmed));
					throw boost::program_options::validation_error(boost::program_options::validation_error::invalid_option_value);
				}

				auto hex_part = trimmed.substr(2);
				uint32_t temporary_device_id = 0;
				auto [ptr, ec] = std::from_chars(hex_part.data(), hex_part.data() + hex_part.size(), temporary_device_id, 16);

				if (std::errc() != ec || ptr != (hex_part.data() + hex_part.size()))
				{
					LogDebug(Channel::Options, std::format("Invalid conversion of emulated device id: invalid hex value -> {}", trimmed));
					throw boost::program_options::validation_error(boost::program_options::validation_error::invalid_option_value);
				}

				if (temporary_device_id > std::numeric_limits<uint8_t>::max())
				{
					LogDebug(Channel::Options, std::format("Invalid conversion of emulated device id: value too large -> {}", trimmed));
					throw boost::program_options::validation_error(boost::program_options::validation_error::invalid_option_value);
				}

				result.push_back(JandyDeviceId{ static_cast<uint8_t>(temporary_device_id) });
			}

			return result;
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
			// No emulated device types specified....ignore.
		}
		else
		{
			auto device_types = ParseDeviceTypes(OPTION_EMULATEDDEVICETYPE->As<std::string>(vm));
			std::vector<Devices::JandyDeviceId> device_ids;

			if (OPTION_EMULATEDDEVICEID->IsPresent(vm))
			{
				device_ids = ParseDeviceIds(OPTION_EMULATEDDEVICEID->As<std::string>(vm));
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

		return std::move(settings);
	}

}
// namespace AqualinkAutomate::Jandy::Options
