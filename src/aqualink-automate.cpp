#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <string>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/stacktrace.hpp>
#include <magic_enum.hpp>

#include "certificates/certificate_management.h"
#include "developer/mock_serial_port.h"
#include "exceptions/exception_optionparsingfailed.h"
#include "exceptions/exception_optionshelporversion.h"
#include "http/webroute_equipment.h"
#include "http/webroute_equipment_buttons.h"
#include "http/webroute_equipment_devices.h"
#include "http/webroute_equipment_version.h"
#include "http/webroute_page_index.h"
#include "http/webroute_page_equipment.h"
#include "http/webroute_page_version.h"
#include "http/webroute_types.h"
#include "http/webroute_version.h"
#include "http/websocket_equipment.h"
#include "http/websocket_equipment_stats.h"
#include "kernel/data_hub.h"
#include "kernel/preferences_hub.h"
#include "kernel/statistics_hub.h"
#include "jandy/devices/iaq_device.h"
#include "jandy/devices/keypad_device.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/devices/pda_device.h"
#include "jandy/devices/serial_adapter_device.h"
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
		// DATA HUB 
		//---------------------------------------------------------------------

		Kernel::DataHub data_hub;
		Kernel::PreferencesHub preferences_hub;
		Kernel::StatisticsHub statistics_hub;

		//---------------------------------------------------------------------
		// JANDY EQUIPMENT
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Starting AqualinkAutomate::JandyEquipment...");

		Equipment::JandyEquipment jandy_equipment(io_context, data_hub, statistics_hub);

		if (!settings.emulated_device.disable_emulation)
		{
			for (const auto& [controller_type, device_type] : settings.emulated_device.emulated_devices)
			{
				LogInfo(Channel::Main, std::format("Enabling controller emulation; type: {}, id: {}", magic_enum::enum_name(controller_type), device_type.Id()));

				auto device_id = std::make_shared<Devices::JandyDeviceType>(device_type);
				std::unique_ptr<Devices::JandyDevice> emulated_device(nullptr);

				switch (controller_type)
				{
				case Devices::JandyEmulatedDeviceTypes::OneTouch:
					emulated_device = std::make_unique<Devices::OneTouchDevice>(io_context, device_id, data_hub, true);
					break;

				case Devices::JandyEmulatedDeviceTypes::RS_Keypad:
					emulated_device = std::make_unique<Devices::KeypadDevice>(io_context, device_id, data_hub, true);
					break;

				case Devices::JandyEmulatedDeviceTypes::IAQ:
					emulated_device = std::make_unique<Devices::IAQDevice>(io_context, device_id, data_hub, true);
					break;

				case Devices::JandyEmulatedDeviceTypes::PDA:
					emulated_device = std::make_unique<Devices::PDADevice>(io_context, device_id, data_hub, true);
					break;

				case Devices::JandyEmulatedDeviceTypes::SerialAdapter:
					emulated_device = std::make_unique<Devices::SerialAdapterDevice>(io_context, device_id, data_hub, true);
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

		HTTP::Server http_server(std::thread::hardware_concurrency());
		std::vector<std::shared_ptr<Interfaces::IShareableRoute>> http_routes;
		
		http_server.enable_timeout(true);
		http_server.enable_response_time(true);
		http_server.enable_http_cache(false);
		http_server.listen(settings.web.bind_address, std::to_string(settings.web.bind_port));
		http_server.set_keep_alive_timeout(30);
		http_server.set_static_dir(settings.web.doc_root);
		
		if (!settings.web.http_server_is_insecure)
		{
			http_server.set_ssl_conf({ settings.web.cert_file.Path(), settings.web.cert_key_file.Path(), "nopassword" });

			/*boost::asio::ssl::context ctx(asio::ssl::context::tls);
			ctx.set_options(
				boost::asio::ssl::context::default_workarounds |	// Implement various bug workarounds
				boost::asio::ssl::context::no_sslv2 |				// Considered insecure
				boost::asio::ssl::context::no_sslv3 |				// Supported but has a known issue ("POODLE bug")
				boost::asio::ssl::context::no_tlsv1 |				// Considered insecure
				boost::asio::ssl::context::no_tlsv1_1 |				// Considered insecure
				boost::asio::ssl::context::single_dh_use |			// Always create a new key using the tmp_dh parameters
				static_cast<long>(SSL_OP_CIPHER_SERVER_PREFERENCE)
			);

			Certificates::LoadWebCertificates(settings.web, ctx);
			http_server.ssl(std::move(ctx));*/
		}

		if (!settings.web.http_content_is_disabled)
		{
			http_routes.push_back(std::make_shared<HTTP::WebRoute_Page_Index>(http_server, data_hub));
			http_routes.push_back(std::make_shared<HTTP::WebRoute_Page_Equipment>(http_server, data_hub));
			http_routes.push_back(std::make_shared<HTTP::WebRoute_Page_Version>(http_server));
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

		http_routes.push_back(std::make_shared<HTTP::WebRoute_Equipment>(http_server, data_hub, statistics_hub));
		http_routes.push_back(std::make_shared<HTTP::WebRoute_Equipment_Buttons>(http_server, data_hub));
		http_routes.push_back(std::make_shared<HTTP::WebRoute_Equipment_Devices>(http_server, data_hub));
		http_routes.push_back(std::make_shared<HTTP::WebRoute_Equipment_Version>(http_server, data_hub));
		http_routes.push_back(std::make_shared<HTTP::WebRoute_Version>(http_server));

		http_routes.push_back(std::make_shared<HTTP::WebSocket_Equipment>(http_server, data_hub));
		http_routes.push_back(std::make_shared<HTTP::WebSocket_Equipment_Stats>(http_server, statistics_hub));

		// This is a non-blocking call; note that the clean-up will trigger a "stop" which terminates the server.
		auto _ = std::async(std::launch::async, [&http_server]() -> void { http_server.run(); });

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
