#include <format>
#include <string>

#include <boost/program_options/value_semantic.hpp>

#include "application/application_defaults.h"
#include "logging/logging.h"
#include "options/options_serial_options.h"
#include "options/helpers/conflicting_options_helper.h"
#include "options/helpers/option_dependency_helper.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::Serial
{

	boost::program_options::options_description OptionsProcessor::Options() const
	{
		boost::program_options::options_description options(SettingsType::AreaName(), Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", SerialOptionsCollection.size(), SettingsType::AreaName()));
		for (auto& option : SerialOptionsCollection)
		{
			options.add((*option)());
		}

		return std::move(options);
	}

	void OptionsProcessor::Validate(const boost::program_options::variables_map& vm) const
	{
		Helper_CheckForConflictingOptions(vm, OPTION_SERIALPORT, OPTION_REMOTESERIALPORT);
		Helper_CheckForConflictingOptions(vm, OPTION_USE_RFC2217, OPTION_USE_RAWTCP);
	}

	std::expected<OptionsProcessor::SettingsType, ErrorCodes::Options_ErrorCodes> OptionsProcessor::Process(boost::program_options::variables_map& vm) const
	{
		SettingsType settings;

		if (OPTION_SERIALPORT->IsPresent(vm)) { settings.serial_port = OPTION_SERIALPORT->As<std::string>(vm); }
		if (OPTION_BAUDRATE->IsPresent(vm)) { settings.baud_rate = OPTION_BAUDRATE->As<uint16_t>(vm); }
		if (OPTION_REMOTESERIALPORT->IsPresent(vm)) { settings.remote_serial_port = OPTION_REMOTESERIALPORT->As<std::string>(vm); }
		if (OPTION_USE_RFC2217->IsPresent(vm)) { settings.use_rfc2217 = OPTION_USE_RFC2217->As<bool>(vm); }
		if (OPTION_USE_RAWTCP->IsPresent(vm)) { settings.use_rawtcp = OPTION_USE_RAWTCP->As<bool>(vm); }

		return std::move(settings);
	}

}
// namespace AqualinkAutomate::Options::Serial
