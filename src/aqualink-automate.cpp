#include <cstdlib>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <crow/app.h>

#include "certificates/certificate_management.h"
#include "developer/mock_serial_port.h"
#include "exceptions/exception_optionparsingfailed.h"
#include "exceptions/exception_optionshelporversion.h"
#include "http/crow_custom_logger.h"
#include "http/webroute_jandyequipment.h"
#include "jandy/jandy.h"
#include "jandy/devices/emulated_onetouch_device.h"
#include "logging/logging.h"
#include "logging/logging_initialise.h"
#include "logging/logging_severity_filter.h"
#include "options/options_initialise.h"
#include "options/options_settings.h"
#include "profiling/profiling.h"
#include "protocol/protocol_handler.h"
#include "serial/serial_initialise.h"
#include "serial/serial_port.h"
#include "signals/signal_awaitable.h"
#include "signals/signal_cleanup.h"
#include "aqualink-automate.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Equipment;
using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Messages;
using namespace AqualinkAutomate::Profiling;
using namespace AqualinkAutomate::Protocol;
using namespace AqualinkAutomate::Serial;
using namespace AqualinkAutomate::Signals;

int main(int argc, char *argv[])
{
    try
    {
        boost::asio::io_context io_context;

        //---------------------------------------------------------------------
        // LOGGING
        //---------------------------------------------------------------------

        Logging::SeverityFiltering::SetGlobalFilterLevel(Severity::Info);
        Logging::Initialise();

        //---------------------------------------------------------------------
        // PROFILING
        //---------------------------------------------------------------------

        if (auto profiler = Factory::ProfilerFactory::Instance().Get(); nullptr != profiler)
        {
            profiler->StartProfiling();
        }

        //---------------------------------------------------------------------
        // OPTIONS
        //---------------------------------------------------------------------

        Options::Settings settings;
        Options::Initialise(settings, argc, argv);

        LogInfo(Channel::Main, "Starting AqualinkAutomate...");

        //---------------------------------------------------------------------
        // SERIAL PORT
        //---------------------------------------------------------------------

        std::shared_ptr<Serial::SerialPort> serial_port;

        if (settings.developer.dev_mode_enabled)
        {
            LogInfo(Channel::Main, "Enabling developer mode");

            if (!settings.developer.replay_file.empty())
            {
                serial_port = std::make_shared<Serial::SerialPort>(io_context, OperatingModes::Mock);
                serial_port->open(settings.developer.replay_file);
            }
            else
            {
                // Normal mode as no developer mode options are enabled.
                serial_port = std::make_shared<Serial::SerialPort>(io_context, OperatingModes::Real);
                Serial::Initialise(settings, serial_port);
            }
        }
        else
        {
            // Normal mode as no developer mode options are enabled.
            serial_port = std::make_shared<Serial::SerialPort>(io_context, OperatingModes::Real);
            Serial::Initialise(settings, serial_port);
        }

        CleanUp::Register({ "Serial", [&serial_port]()->void { serial_port->cancel(); serial_port->close(); } });

        //---------------------------------------------------------------------
        // JANDY EQUIPMENT
        //---------------------------------------------------------------------

        Generators::JandyMessageGenerator jandy_message_generator;
        Generators::JandyRawDataGenerator jandy_rawdata_generator;

        Protocol::ProtocolHandler protocol_handler(io_context, *serial_port, jandy_message_generator, jandy_rawdata_generator);
        boost::asio::co_spawn(io_context, protocol_handler.Run(), boost::asio::detached);

        Equipment::JandyEquipment jandy_equipment(io_context, protocol_handler);
        CleanUp::Register({ "JandyEquipment", [&jandy_equipment]()->void { jandy_equipment.Stop(); } });

        auto onetouch_emulated = std::make_unique<Devices::OneTouchDevice_Emulated>(io_context, 0x41);
        jandy_equipment.AddEmulatedDevice(std::move(onetouch_emulated));

        //FIXME -> blocks coroutines!!!  boost::asio::co_spawn(io_context, jandy_equipment.Run(), boost::asio::detached);

        //---------------------------------------------------------------------
        // WEB SERVER
        //---------------------------------------------------------------------

        HTTP::CrowCustomLogger crow_custom_logger;
        crow::logger::setHandler(&crow_custom_logger);

        crow::SimpleApp http_server;
        http_server.bindaddr(settings.web.bind_address).port(settings.web.bind_port);

        if (!settings.web.http_server_is_insecure)
        {
            boost::asio::ssl::context ctx(boost::asio::ssl::context::tls);
            ctx.set_options(
                boost::asio::ssl::context::default_workarounds |	// Implement various bug workarounds
                boost::asio::ssl::context::no_sslv2 |				// Considered insecure
                boost::asio::ssl::context::no_sslv3 |				// Supported but has a known issue ("POODLE bug")
                boost::asio::ssl::context::no_tlsv1 |				// Considered insecure
                boost::asio::ssl::context::no_tlsv1_1 |				// Considered insecure
                boost::asio::ssl::context::single_dh_use |			// Always create a new key using the tmp_dh parameters
                SSL_OP_CIPHER_SERVER_PREFERENCE
            );

            Certificates::LoadWebCertificates(settings.web, ctx);
            http_server.ssl(std::move(ctx));
        }

        HTTP::WebRoute_JandyEquipment jandy_web_route(http_server, jandy_equipment);

        CleanUp::Register({ "WebServer", [&http_server]()->void { http_server.stop(); } });
        auto httpsrv_async = http_server.run_async();

        //---------------------------------------------------------------------
        // SIGNALS
        //---------------------------------------------------------------------

        boost::asio::signal_set ss(io_context);
        Signal_Awaitable(ss);

        io_context.run();

        if (auto profiler = Factory::ProfilerFactory::Instance().Get(); nullptr != profiler)
        {
            profiler->StopProfiling();
        }

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
