#include <format>

#include <boost/program_options.hpp>
#include <magic_enum.hpp>

#include "exceptions/exception_optionparsingfailed.h"
#include "logging/logging.h"
#include "options/options_app_options.h"
#include "options/options_developer_options.h"
#include "options/options_emulated_device_options.h" 
#include "options/options_initialise.h"
#include "options/options_serial_options.h"
#include "options/options_web_options.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options
{

	void Initialise(Settings& settings, int argc, char* argv[])
	{
		try
		{
			boost::program_options::options_description cmdline_options;

			LogDebug(Channel::Options, "Configuring application options");

			cmdline_options
				.add(App::Options())
				.add(Developer::Options())
				.add(Emulated::Options())
				.add(Serial::Options())
				.add(Web::Options());

			LogDebug(Channel::Options, "Parsing application options via command line");

			boost::program_options::command_line_parser parser(argc, argv);
			boost::program_options::variables_map variables;
			boost::program_options::store(parser.options(cmdline_options).run(), variables);
			boost::program_options::notify(variables);

			// Handle the help/version options before anything else.
			App::HandleHelp(variables, cmdline_options);
			App::HandleVersion(variables);

			// Validate that there are no conflicts or missing option dependencies
			App::ValidateOptions(variables);
			Developer::ValidateOptions(variables);
			Emulated::ValidateOptions(variables);
			Serial::ValidateOptions(variables);
			Web::ValidateOptions(variables);

			// Handle the various options and configure the application.
			settings.app = App::HandleOptions(variables);
			settings.developer = Developer::HandleOptions(variables);
			settings.emulated_device = Emulated::HandleOptions(variables);
			settings.serial = Serial::HandleOptions(variables);
			settings.web = Web::HandleOptions(variables);
		}
		catch (const boost::program_options::invalid_syntax& po_is)
		{
			LogFatal(Channel::Options, std::format("User incorrectly specified options on the command line; type was -> {}, error was -> {}.", magic_enum::enum_name(po_is.kind()), po_is.what()));
			///FIXME Show the help.
			throw Exceptions::OptionParsingFailed();

		}
		catch (const boost::program_options::error& po_err)
		{
			LogFatal(Channel::Options, std::format("Triggering parsing failure exception (unknown error); error was -> {}", po_err.what()));
			///FIXME Show the help.
			throw Exceptions::OptionParsingFailed();
		}
	}

}
// namespace AqualinkAutomate::Options
