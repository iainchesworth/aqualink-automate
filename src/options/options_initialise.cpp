#include <boost/program_options.hpp>

#include "exceptions/exception_optionparsingfailed.h"
#include "logging/logging.h"
#include "options/options_app_options.h"
#include "options/options_initialise.h"
#include "options/options_serial_options.h"
#include "options/options_web_options.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options
{

	void Initialise(int argc, char* argv[])
	{
		try
		{
			boost::program_options::options_description cmdline_options;

			LogDebug(Channel::Options, "Configuring application options");

			cmdline_options
				.add(App::Options())
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

			// Handle the various options and configure the application.
			App::HandleOptions(variables);
			Serial::HandleOptions(variables);
			Web::HandleOptions(variables);
		}
		catch (const boost::program_options::error& po_err)
		{
			LogDebug(Channel::Options, "Triggering parsing failure exception");
			throw Exceptions::OptionParsingFailed();
		}
	}

}
// namespace AqualinkAutomate::Options
