#include <thread>
#include <vector>

#include <boost/asio.hpp>
#include <boost/cobalt.hpp>
#include <boost/stacktrace.hpp>
#include <magic_enum.hpp>

#include "certificates/certificate_management.h"
#include "coroutines/asynchronous_executor.h"
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
#include "aqualink-automate.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Equipment;
using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Messages;
using namespace AqualinkAutomate::Profiling;
using namespace AqualinkAutomate::Protocol;
using namespace AqualinkAutomate::Serial;

boost::cobalt::main co_main(int argc, char* argv[])
{
	try
	{
		boost::cobalt::wait_group server_tasks_wg;

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

		auto serial_port_executor = co_await boost::cobalt::this_coro::executor;
		std::shared_ptr<Serial::SerialPort> serial_port;

		if (settings.developer.dev_mode_enabled)
		{
			LogInfo(Channel::Main, "Enabling developer mode");

			if (!settings.developer.replay_file.empty())
			{
				serial_port = std::make_shared<Serial::SerialPort>(serial_port_executor, hub_locator, OperatingModes::Mock);
				serial_port->open(settings.developer.replay_file);
			}
			else
			{
				// Normal mode as no developer mode options are enabled.
				serial_port = std::make_shared<Serial::SerialPort>(serial_port_executor, hub_locator, OperatingModes::Real);
				Serial::Initialise(settings, serial_port);
			}
		}
		else
		{
			// Normal mode as no developer mode options are enabled.
			serial_port = std::make_shared<Serial::SerialPort>(serial_port_executor, hub_locator, OperatingModes::Real);
			Serial::Initialise(settings, serial_port);
		}

		//---------------------------------------------------------------------
		// JANDY EQUIPMENT
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Starting AqualinkAutomate::JandyEquipment...");

		equipment_hub->AddEquipment(std::make_unique<Equipment::JandyEquipment>(hub_locator));

		if (!settings.emulated_device.disable_emulation)
		{
			for (const auto& [controller_type, device_type] : settings.emulated_device.emulated_devices)
			{
				LogInfo(Channel::Main, std::format("Enabling controller emulation; type: {}, id: {}", magic_enum::enum_name(controller_type), device_type.Id()));

				auto device_id = std::make_shared<Devices::JandyDeviceType>(device_type);

				switch (controller_type)
				{
				case Devices::JandyEmulatedDeviceTypes::OneTouch:
					equipment_hub->AddDevice(std::move(std::make_unique<Devices::OneTouchDevice>(device_id, hub_locator, true)));
					break;

				case Devices::JandyEmulatedDeviceTypes::RS_Keypad:
					equipment_hub->AddDevice(std::move(std::make_unique<Devices::KeypadDevice>(device_id, hub_locator, true)));
					break;

				case Devices::JandyEmulatedDeviceTypes::IAQ:
					equipment_hub->AddDevice(std::move(std::make_unique<Devices::IAQDevice>(device_id, hub_locator, true)));
					break;

				case Devices::JandyEmulatedDeviceTypes::PDA:
					equipment_hub->AddDevice(std::move(std::make_unique<Devices::PDADevice>(device_id, hub_locator, true)));
					break;

				case Devices::JandyEmulatedDeviceTypes::SerialAdapter:
					equipment_hub->AddDevice(std::move(std::make_unique<Devices::SerialAdapterDevice>(device_id, hub_locator, true)));
					break;

				case Devices::JandyEmulatedDeviceTypes::Unknown:
				default:
					LogWarning(Channel::Main, "Unknown emulated device type; cannot create controller device");
				}
			}
		}

		//---------------------------------------------------------------------
		// PROTOCOL HANDLER
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Starting AqualinkAutomate::ProtocolHandler...");

		server_tasks_wg.push_back(Protocol::ProtocolHandler_ReadOp(*serial_port));
		server_tasks_wg.push_back(Protocol::ProtocolHandler_WriteOp<Messages::JandyMessage_Ack>(*serial_port));

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

		boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tls_server);

		if (settings.web.https_server_is_enabled)
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

			Certificates::LoadSslCertificates(settings.web, ssl_context);

			server_tasks_wg.push_back(HTTP::Listener(boost::asio::ip::tcp::endpoint{ boost::asio::ip::make_address(settings.web.bind_address),settings.web.https_port }, ssl_context));
		}

		if (settings.web.http_server_is_enabled)
		{
			server_tasks_wg.push_back(HTTP::Listener(boost::asio::ip::tcp::endpoint{ boost::asio::ip::make_address(settings.web.bind_address),settings.web.http_port }));
		}

		//---------------------------------------------------------------------
		// START APPLICATION
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Starting AqualinkAutomate...");

		co_await server_tasks_wg.wait();

		//---------------------------------------------------------------------
		// STOP APPLICATION
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Stopping AqualinkAutomate...");

		if (auto profiler = Factory::ProfilerFactory::Instance().Get(); nullptr != profiler)
		{
			profiler->StopProfiling();
		}
		
		co_return EXIT_SUCCESS;
	}
	catch (const Exceptions::OptionsHelpOrVersion&)
	{
		// Nothing happens since the user has been informed of the help/version information.
		// Just terminate as if nothing had happened.
		co_return EXIT_SUCCESS;
	}
	catch (const Exceptions::OptionParsingFailed&)
	{
		// Nothing happens since the user has been informed of the option parsing error.
		// Just terminate as if nothing had happened.
		co_return EXIT_SUCCESS;
	}
	catch (const Exceptions::GenericAqualinkException& ex_gae)
	{
		const auto& sl = ex_gae.Where();
		const auto& st = ex_gae.StackTrace();

		LogFatal(
			Channel::Main,
			std::format(
				"Unknown exception occurred at {} ({}:{})...terminating!  Message: {}",
				sl.file_name(),
				sl.line(),
				sl.column(),
				ex_gae.What()
			)
		);

		for (const boost::stacktrace::frame& frame : st)
		{
			LogDebug(Channel::Main, std::format("{}, {}({})", frame.name(), frame.source_file().empty() ? "Unknown File" : frame.source_file(), frame.source_line()));
		}

		co_return EXIT_FAILURE;
	}
	catch (const boost::system::system_error& err)
	{
		LogFatal(Channel::Main, std::format("Unknown exception occurred...terminating!  Message: {}", err.what()));
		co_return EXIT_FAILURE;
	}
	catch (const std::exception& err)
	{
		LogFatal(Channel::Main, std::format("Unknown exception occurred...terminating!  Message: {}", err.what()));
		co_return EXIT_FAILURE;
	}
}
