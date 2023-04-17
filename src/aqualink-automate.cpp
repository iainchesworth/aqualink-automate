#include <cstdlib>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>

#include "developer/mock_serial_port.h"
#include "exceptions/exception_optionparsingfailed.h"
#include "exceptions/exception_optionshelporversion.h"
#include "jandy/jandy.h"
#include "logging/logging.h"
#include "logging/logging_initialise.h"
#include "logging/logging_severity_filter.h"
#include "options/options_initialise.h"
#include "options/options_settings.h"
#include "protocol/protocol_handler.h"
#include "serial/serial_initialise.h"
#include "serial/serial_port.h"
#include "signals/signal_awaitable.h"
#include "signals/signal_cleanup.h"
#include "aqualink-automate.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Bridges;
using namespace AqualinkAutomate::Equipment;
using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Messages;
using namespace AqualinkAutomate::Protocol;
using namespace AqualinkAutomate::Serial;
using namespace AqualinkAutomate::Signals;

int main(int argc, char *argv[])
{
    try
    {
        boost::asio::io_context io_context;

        Logging::SeverityFiltering::SetGlobalFilterLevel(Severity::Info);
        Logging::Initialise();

        Options::Settings settings;
        Options::Initialise(settings, argc, argv);

        LogInfo(Channel::Main, "Starting AqualinkAutomate...");

        std::shared_ptr<Serial::SerialPort> serial_port;

        if (settings.developer.dev_mode_enabled)
        {
            LogInfo(Channel::Main, "Enabling developer mode");
            
            if (!settings.developer.replay_file.empty())
            {
                serial_port = std::make_shared<Serial::SerialPort>(io_context, OperatingModes::Mock);
                serial_port->open(settings.developer.replay_file);
            }
        }
        else
        {
            // Normal mode as no developer mode options are enabled.
            serial_port = std::make_shared<Serial::SerialPort>(io_context, OperatingModes::Real);
            Serial::Initialise(settings, serial_port);
        }

        CleanUp::Register({ "Serial", [&serial_port]()->void { serial_port->cancel(); serial_port->close(); } });

        Generators::JandyMessageGenerator jandy_message_generator;
        Bridges::Bridge_JandyMessages jandy_message_bridge;

        Protocol::ProtocolHandler protocol_handler(io_context, *serial_port, jandy_message_generator, jandy_message_bridge);
        boost::asio::co_spawn(io_context, protocol_handler.Run(), boost::asio::detached);

        Equipment::JandyEquipment jandy_equipment(io_context, protocol_handler, jandy_message_bridge);
        boost::asio::co_spawn(io_context, jandy_equipment.Run(), boost::asio::detached);

        boost::asio::signal_set ss(io_context);
        boost::asio::co_spawn(io_context, Signal_Awaitable(ss), boost::asio::detached);

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
