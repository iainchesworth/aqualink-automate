#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <string>

#include <asio/ssl/context.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/stacktrace.hpp>
#include <crow/app.h>
#include <magic_enum.hpp>

#include "certificates/certificate_management.h"
#include "developer/mock_serial_port.h"
#include "exceptions/exception_optionparsingfailed.h"
#include "exceptions/exception_optionshelporversion.h"
#include "http/crow_custom_logger.h"
#include "http/webroute_jandyequipment.h"
#include "http/webroute_jandyequipment_buttons.h"
#include "http/webroute_jandyequipment_devices.h"
#include "http/webroute_jandyequipment_version.h"
#include "http/webroute_page_index.h"
#include "http/webroute_page_jandyequipment.h"
#include "http/webroute_page_version.h"
#include "http/webroute_version.h"
#include "http/websocket_jandyequipment.h"
#include "http/websocket_jandyequipment_stats.h"
#include "jandy/config/jandy_config.h"
#include "jandy/devices/iaq_device.h"
#include "jandy/devices/keypad_device.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/devices/pda_device.h"
#include "jandy/equipment/jandy_equipment.h"
#include "jandy/messages/jandy_message_ack.h"
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

int main(int argc, char* argv[])
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

		LogInfo(Channel::Main, "Starting AqualinkAutomate::JandyEquipment...");

		Config::JandyConfig jandy_config;
		Equipment::JandyEquipment jandy_equipment(io_context, jandy_config);

		if (!settings.emulated_device.disable_emulation)
		{
			LogInfo(
				Channel::Main,
				std::format(
					"Enabling controller emulation; type: {}, id: {}",
					magic_enum::enum_name(settings.emulated_device.controller_type),
					settings.emulated_device.device_type.Id()
				)
			);

			std::unique_ptr<Devices::JandyDevice> emulated_device(nullptr);
			switch (settings.emulated_device.controller_type)
			{
			case Devices::JandyEmulatedDeviceTypes::OneTouch:
				emulated_device = std::make_unique<Devices::OneTouchDevice>(io_context, settings.emulated_device.device_type, jandy_config, true);
				break;

			case Devices::JandyEmulatedDeviceTypes::RS_Keypad:
				emulated_device = std::make_unique<Devices::KeypadDevice>(io_context, settings.emulated_device.device_type, jandy_config, true);
				break;

			case Devices::JandyEmulatedDeviceTypes::IAQ:
				emulated_device = std::make_unique<Devices::IAQDevice>(io_context, settings.emulated_device.device_type, jandy_config, true);
				break;

			case Devices::JandyEmulatedDeviceTypes::PDA:
				emulated_device = std::make_unique<Devices::PDADevice>(io_context, settings.emulated_device.device_type, jandy_config, true);
				break;

			case Devices::JandyEmulatedDeviceTypes::Unknown:
			default:
				LogWarning(Channel::Main, "Unknown emulated device type; cannot create controller device");
			}

			if (emulated_device)
			{
				jandy_equipment.AddEmulatedDevice(std::move(emulated_device));
			}
		}

		CleanUp::Register({ "JandyEquipment", [&jandy_equipment]()->void { /* DO NOTHING */ } });

		//---------------------------------------------------------------------
		// PROTOCOL HANDLER
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Starting AqualinkAutomate::ProtocolHandler...");

		Protocol::ProtocolHandler protocol_handler(io_context, *serial_port);
		protocol_handler.RegisterPublishableMessage<Messages::JandyMessage_Ack>();

		// This is a non-blocking call as it posts the step into the io context; note that the clean-up will trigger a "stop".
		protocol_handler.Run();

		CleanUp::Register({ "Protocol Handler Thread", [&io_context]() -> void { /* DO NOTHING */ } });

		//---------------------------------------------------------------------
		// WEB SERVER
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Starting AqualinkAutomate::HttpServer...");

		HTTP::CrowCustomLogger crow_custom_logger;
		crow::logger::setHandler(&crow_custom_logger);

		crow::mustache::set_global_base(settings.web.doc_root);

		crow::SimpleApp http_server;
		http_server.loglevel(crow::LogLevel::Debug); // Filtering is handled by the aqualink-automate logger.
		http_server.bindaddr(settings.web.bind_address).port(settings.web.bind_port);

		if (!settings.web.http_server_is_insecure)
		{
			asio::ssl::context ctx(asio::ssl::context::tls);
			ctx.set_options(
				asio::ssl::context::default_workarounds |	// Implement various bug workarounds
				asio::ssl::context::no_sslv2 |				// Considered insecure
				asio::ssl::context::no_sslv3 |				// Supported but has a known issue ("POODLE bug")
				asio::ssl::context::no_tlsv1 |				// Considered insecure
				asio::ssl::context::no_tlsv1_1 |			// Considered insecure
				asio::ssl::context::single_dh_use |			// Always create a new key using the tmp_dh parameters
				static_cast<long>(SSL_OP_CIPHER_SERVER_PREFERENCE)
			);

			Certificates::LoadWebCertificates(settings.web, ctx);
			http_server.ssl(std::move(ctx));
		}

		if (!settings.web.http_content_is_disabled)
		{
			HTTP::WebRoute_Page_Index page_index(http_server, jandy_equipment);
			HTTP::WebRoute_Page_JandyEquipment page_je(http_server, jandy_equipment);
			HTTP::WebRoute_Page_Version page_version(http_server);
		}

		// Routes are configured as follows
		//
		//     /api
		//     /api/equipment
		//     /api/equipment/buttons
		//     /api/equipment/devices
		//     /api/equipment/stats		<-- NOT IMPLEMENTED YET (but returned as part of "equipment" payload)
		//     /api/equipment/version
		//     
		//     /ws
		//     /ws/equipment
		//     /ws/equipment/stats
		//

		HTTP::WebRoute_JandyEquipment route_je(http_server, jandy_equipment);
		HTTP::WebRoute_JandyEquipment_Buttons route_je_buttons(http_server, jandy_equipment);
		HTTP::WebRoute_JandyEquipment_Devices route_je_devices(http_server, jandy_equipment);
		HTTP::WebRoute_JandyEquipment_Version route_je_version(http_server, jandy_equipment);
		HTTP::WebRoute_Version route_version(http_server);

		HTTP::WebSocket_JandyEquipment websocket_je(http_server, jandy_equipment);
		HTTP::WebSocket_JandyEquipment_Stats websocket_je_stats(http_server, jandy_equipment);

		// Check that the routes are configured correctly.
		http_server.validate();

		// This is a non-blocking call; note that the clean-up will trigger a "stop" which terminates the server.
		auto http_server_instance = http_server.run_async();

		CleanUp::Register({ "Web Server Thread", [&http_server]() -> void { http_server.stop(); } });

		//---------------------------------------------------------------------
		// SIGNALS
		//---------------------------------------------------------------------

		boost::asio::signal_set ss(io_context);
		Signal_Awaitable(ss);

		//---------------------------------------------------------------------
		// START APPLICATION
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Starting AqualinkAutomate...");

		io_context.run();

		//---------------------------------------------------------------------
		// STOP APPLICATION
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Stopping AqualinkAutomate...");

		if (auto profiler = Factory::ProfilerFactory::Instance().Get(); nullptr != profiler)
		{
			profiler->StopProfiling();
		}
		
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
	catch (const Exceptions::GenericAqualinkException& ex_gae)
	{
		LogFatal(Channel::Main, std::format("Unknown exception occurred...terminating!  Message: {}", ex_gae.what()));

		if (const boost::stacktrace::stacktrace* st = boost::get_error_info<Exceptions::Traced>(ex_gae); nullptr != st)
		{
			for (const boost::stacktrace::frame& frame : *st)
			{
				LogDebug(Channel::Main, std::format("{}, {}({})", frame.name(), frame.source_file().empty() ? "Unknown File" : frame.source_file(), frame.source_line()));
			}
		}		

		return EXIT_FAILURE;
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
