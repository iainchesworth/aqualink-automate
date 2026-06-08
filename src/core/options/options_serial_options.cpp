#include <string>

#include <boost/program_options/value_semantic.hpp>

#include "application/application_defaults.h"
#include "options/options_serial_options.h"
#include "options/helpers/build_options_description.h"
#include "options/helpers/conflicting_options_helper.h"
#include "options/helpers/option_dependency_helper.h"

using namespace AqualinkAutomate;

namespace AqualinkAutomate::Options::Serial
{

	boost::program_options::options_description OptionsProcessor::Options() const
	{
		return BuildOptionsDescription(SettingsType::AreaName(), SerialOptionsCollection);
	}

	void OptionsProcessor::Validate(const boost::program_options::variables_map& vm) const
	{
		Helper_CheckForConflictingOptions(vm, OPTION_SERIALPORT, OPTION_REMOTESERIALPORT);
		Helper_CheckForConflictingOptions(vm, OPTION_USE_RFC2217, OPTION_USE_RAWTCP);

		// --rfc2217 and --no-rfc2217 contradict each other.
		Helper_CheckForConflictingOptions(vm, OPTION_USE_RFC2217, OPTION_NO_RFC2217);
		// Disabling RFC2217 while asking for raw TCP is redundant, but explicitly
		// combining --no-rfc2217 with --rawtcp is contradictory (one says "plain
		// socket via the RFC2217 transport disabled", the other selects raw TCP).
		Helper_CheckForConflictingOptions(vm, OPTION_NO_RFC2217, OPTION_USE_RAWTCP);
	}

	std::expected<OptionsProcessor::SettingsType, ErrorCodes::Options_ErrorCodes> OptionsProcessor::Process(boost::program_options::variables_map& vm) const
	{
		SettingsType settings;

		if (OPTION_SERIALPORT->IsPresent(vm)) { settings.serial_port = OPTION_SERIALPORT->As<std::string>(vm); }
		if (OPTION_BAUDRATE->IsPresent(vm)) { settings.baud_rate = OPTION_BAUDRATE->As<uint16_t>(vm); }
		if (OPTION_REMOTESERIALPORT->IsPresent(vm)) { settings.remote_serial_port = OPTION_REMOTESERIALPORT->As<std::string>(vm); }

		// RFC2217 is on by default for remote ports. Honour an explicit --rfc2217,
		// but let --no-rfc2217 turn it off (the old default_value(true) bool_switch
		// was permanently true and could never be disabled).
		if (OPTION_USE_RFC2217->IsPresent(vm)) { settings.use_rfc2217 = OPTION_USE_RFC2217->As<bool>(vm); }
		if (OPTION_NO_RFC2217->IsPresentAndNotJustDefaulted(vm) && OPTION_NO_RFC2217->As<bool>(vm)) { settings.use_rfc2217 = false; }

		if (OPTION_USE_RAWTCP->IsPresent(vm)) { settings.use_rawtcp = OPTION_USE_RAWTCP->As<bool>(vm); }

		return settings;
	}

}
// namespace AqualinkAutomate::Options::Serial
