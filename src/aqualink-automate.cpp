#include <cstdlib>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>

#include "exceptions/exception_optionparsingfailed.h"
#include "exceptions/exception_optionshelporversion.h"
#include "logging/logging.h"
#include "logging/logging_initialise.h"
#include "logging/logging_severity_filter.h"
#include "options/options_initialise.h"
#include "protocol/protocol_jandy.h"
#include "serial/serial_port.h"
#include "signals/signal_awaitable.h"
#include "aqualink-automate.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Protocol;
using namespace AqualinkAutomate::Serial;
using namespace AqualinkAutomate::Signals;

int main(int argc, char *argv[])
{
    try
    {
        boost::asio::io_context io_context;

        Logging::SeverityFiltering::SetGlobalFilterLevel(Severity::Trace);
        Logging::Initialise();

        Options::Initialise(argc, argv);

        boost::asio::signal_set ss(io_context);
        boost::asio::co_spawn(io_context, Signal_Awaitable(ss), boost::asio::detached);

        LogInfo(Channel::Main, "Starting AqualinkAutomate...");

        io_context.run();

        LogInfo(Channel::Main, "Stopping AqualinkAutomate...");

        return EXIT_SUCCESS;
    }
    catch (const Exceptions::OptionsHelpOrVersion& ex_ohov)
    {
        // Nothing happens since the user has been informed of the help/version information.
        // Just terminate as if nothing had happened.
        return EXIT_SUCCESS;
    }
    catch (const Exceptions::OptionParsingFailed& ex_opf)
    {
        // Nothing happens since the user has been informed of the option parsing error.
        // Just terminate as if nothing had happened.
        return EXIT_SUCCESS;
    }
    catch (const boost::system::system_error& err)
    {
        LogFatal(Channel::Main, std::format("Unknown exception occurred...terminating!  Message: {}", err.what()));
        return EXIT_FAILURE;
    }
    catch (const std::exception& err)
    {
        LogFatal(Channel::Main, std::format("Unknown exception occurred...terminating!  Message: {}", err.what()));
        return EXIT_FAILURE;
    }
}
