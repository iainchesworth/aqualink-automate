#include <boost/asio/ssl/context.hpp>
#include <boost/stacktrace.hpp>
#include <magic_enum.hpp>

#include "certificates/certificate_management.h"
#include "developer/firewall_manager.h"
#include "developer/mock_serial_port.h"
#include "exceptions/exception_optionparsingfailed.h"
#include "exceptions/exception_optionshelporversion.h"
#include "http/server/static_file_handler.h"
#include "http/webroute_equipment.h"
#include "http/webroute_equipment_button.h"
#include "http/webroute_equipment_buttons.h"
#include "http/webroute_equipment_devices.h"
#include "http/webroute_equipment_version.h"
#include "http/webroute_page_index.h"
#include "http/webroute_page_equipment.h"
#include "http/webroute_page_version.h"
#include "http/webroute_version.h"
#include "http/websocket_equipment.h"
#include "http/websocket_equipment_stats.h"
#include "http/server/listener.h"
#include "http/server/routing/routing.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/hub_locator.h"
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
#include "types/asynchronous_executor.h"
#include "types/asynchronous_threadpool.h"
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
		Types::AsyncThreadPool thread_pool(std::thread::hardware_concurrency());

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
		// INFORMATION DISTRIBUTION HUBS 
		//---------------------------------------------------------------------

		Kernel::HubLocator hub_locator;

		auto data_hub = std::make_shared<Kernel::DataHub>();
		auto equipment_hub = std::make_shared<Kernel::EquipmentHub>();
		auto preferences_hub = std::make_shared<Kernel::PreferencesHub>();
		auto statistics_hub = std::make_shared<Kernel::StatisticsHub>();

		hub_locator.Register(data_hub).Register(equipment_hub).Register(preferences_hub).Register(statistics_hub);

		//---------------------------------------------------------------------
		// SERIAL PORT
		//---------------------------------------------------------------------

		std::shared_ptr<Serial::SerialPort> serial_port;

		if (settings.developer.dev_mode_enabled)
		{
			LogInfo(Channel::Main, "Enabling developer mode");

			if (!settings.developer.replay_file.empty())
			{
				serial_port = std::make_shared<Serial::SerialPort>(thread_pool.get_executor(), hub_locator, OperatingModes::Mock);
				serial_port->open(settings.developer.replay_file);
			}
			else
			{
				// Normal mode as no developer mode options are enabled.
				serial_port = std::make_shared<Serial::SerialPort>(thread_pool.get_executor(), hub_locator, OperatingModes::Real);
				Serial::Initialise(settings, serial_port);
			}
		}
		else
		{
			// Normal mode as no developer mode options are enabled.
			serial_port = std::make_shared<Serial::SerialPort>(thread_pool.get_executor(), hub_locator, OperatingModes::Real);
			Serial::Initialise(settings, serial_port);
		}

		CleanUp::Register({ "Serial", [&serial_port]()->void { serial_port->cancel(); serial_port->close(); } });

		//---------------------------------------------------------------------
		// JANDY EQUIPMENT
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Starting AqualinkAutomate::JandyEquipment...");

		auto jandy_equipment = std::make_shared<Equipment::JandyEquipment>(thread_pool.get_executor(), hub_locator);
		equipment_hub->AddEquipment(jandy_equipment);

		if (!settings.emulated_device.disable_emulation)
		{
			for (const auto& [controller_type, device_type] : settings.emulated_device.emulated_devices)
			{
				LogInfo(Channel::Main, std::format("Enabling controller emulation; type: {}, id: {}", magic_enum::enum_name(controller_type), device_type.Id()));

				auto device_id = std::make_shared<Devices::JandyDeviceType>(device_type);

				switch (controller_type)
				{
				case Devices::JandyEmulatedDeviceTypes::OneTouch:
					equipment_hub->AddDevice(std::make_shared<Devices::OneTouchDevice>(thread_pool.get_executor(), device_id, hub_locator, true));
					break;

				case Devices::JandyEmulatedDeviceTypes::RS_Keypad:
					equipment_hub->AddDevice(std::make_shared<Devices::KeypadDevice>(thread_pool.get_executor(), device_id, hub_locator, true));
					break;

				case Devices::JandyEmulatedDeviceTypes::IAQ:
					equipment_hub->AddDevice(std::make_shared<Devices::IAQDevice>(thread_pool.get_executor(), device_id, hub_locator, true));
					break;

				case Devices::JandyEmulatedDeviceTypes::PDA:
					equipment_hub->AddDevice(std::make_shared<Devices::PDADevice>(thread_pool.get_executor(), device_id, hub_locator, true));
					break;

				case Devices::JandyEmulatedDeviceTypes::SerialAdapter:
					equipment_hub->AddDevice(std::make_shared<Devices::SerialAdapterDevice>(thread_pool.get_executor(), device_id, hub_locator, true));
					break;

				case Devices::JandyEmulatedDeviceTypes::Unknown:
				default:
					LogWarning(Channel::Main, "Unknown emulated device type; cannot create controller device");
				}
			}
		}

		CleanUp::Register({ "JandyEquipment", [&jandy_equipment]()->void { /* DO NOTHING */ } });

		//---------------------------------------------------------------------
		// PROTOCOL HANDLER
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Starting AqualinkAutomate::ProtocolHandler...");

		Protocol::ProtocolHandler protocol_handler(*serial_port);
		protocol_handler.RegisterPublishableMessage<Messages::JandyMessage_Ack>();

		// This is a non-blocking call as it posts the step into the io context; note that the clean-up will trigger a "stop".
		protocol_handler.Run();

		CleanUp::Register({ "Protocol Handler", []() -> void { /* DO NOTHING */ } });

		//---------------------------------------------------------------------
		// WEB SERVER
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Starting AqualinkAutomate::HttpServer...");

		Developer::FirewallUtils::CheckAndConfigureExceptions();

		if (!settings.web.http_content_is_disabled)
		{
            HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Page_Index>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Page_Equipment>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Page_Version>());
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

		HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Equipment>(hub_locator));
		HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Equipment_Button>(hub_locator));
		HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Equipment_Buttons>(hub_locator));
		HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Equipment_Devices>(hub_locator));
		HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Equipment_Version>(hub_locator));
		HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Version>());

		HTTP::Routing::Add(std::make_unique<HTTP::WebSocket_Equipment>(hub_locator));
		HTTP::Routing::Add(std::make_unique<HTTP::WebSocket_Equipment_Stats>(hub_locator));

		HTTP::Routing::StaticHandler(HTTP::StaticFileHandler("/", settings.web.doc_root));

		boost::asio::ssl::context ssl_context(boost::asio::ssl::context::sslv23);

		if (!settings.web.http_server_is_insecure)
		{
			ssl_context.set_options(
				boost::asio::ssl::context::default_workarounds |	// Implement various bug workarounds
				boost::asio::ssl::context::no_sslv2 |				// Considered insecure
				boost::asio::ssl::context::no_sslv3 |				// Supported but has a known issue ("POODLE bug")
				boost::asio::ssl::context::no_tlsv1 |				// Considered insecure
				boost::asio::ssl::context::no_tlsv1_1 |				// Considered insecure
				boost::asio::ssl::context::single_dh_use |			// Always create a new key using the tmp_dh parameters
				static_cast<long>(SSL_OP_CIPHER_SERVER_PREFERENCE)
			);

			const std::string cipher_list
			{
				"TLS_AES_128_GCM_SHA256:"
				"TLS_AES_256_GCM_SHA384:"
				"TLS_CHACHA20_POLY1305_SHA256:"
				"ECDHE-RSA-AES128-GCM-SHA256:"
				"ECDHE-RSA-AES256-GCM-SHA384"
			};

			SSL_CTX_set_cipher_list(ssl_context.native_handle(), cipher_list.c_str());

			Certificates::LoadSslCertificates(settings.web, ssl_context);
		}

		boost::asio::ip::tcp::endpoint bind_endpoint(boost::asio::ip::address::from_string(settings.web.bind_address), settings.web.bind_port);
        auto http_server = std::make_shared<HTTP::Listener>(thread_pool.get_executor(), bind_endpoint, ssl_context);

		// This is a non-blocking call as it posts the step into the io context; note that the clean-up will trigger a "stop".
		http_server->Run();

		CleanUp::Register({ "Web Server", []() -> void { /* DO NOTHING */ }});

		//---------------------------------------------------------------------
		// SIGNALS
		//---------------------------------------------------------------------

		boost::asio::signal_set ss(thread_pool.get_executor());
		Signal_Awaitable(ss);

		//---------------------------------------------------------------------
		// START APPLICATION
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Starting AqualinkAutomate...");
		thread_pool.join();

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
