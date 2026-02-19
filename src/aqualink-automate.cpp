// Standard library
#include <chrono>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stacktrace>
#include <thread>
#include <vector>

// Third-party
#include <boost/asio.hpp>
#include <boost/stacktrace.hpp>
#include <magic_enum/magic_enum.hpp>

// Core — infrastructure
#include "certificates/certificate_management.h"
#include "exceptions/exception_optionparsingfailed.h"
#include "exceptions/exception_optionshelporversion.h"
#include "interfaces/icommanddispatcher.h"
#include "interfaces/iserialportimpl.h"
#include "logging/logging.h"
#include "logging/logging_initialise.h"
#include "logging/logging_severity_filter.h"
#include "options/options.h"
#include "profiling/profiling.h"
#include "version/version.h"

// Core — kernel
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/hub_locator.h"
#include "kernel/preferences_hub.h"
#include "kernel/statistics_hub.h"

// Core — developer tools
#include "developer/firewall_manager.h"
#include "developer/mock_serial_port_impl.h"
#include "developer/recording_serial_port_impl.h"

// Core — HTTP server and routes
#include "http/server/http_server.h"
#include "http/server/static_file_handler.h"
#include "http/server/routing/routing.h"
#include "http/webroute_diagnostics_logging.h"
#include "http/webroute_equipment.h"
#include "http/webroute_equipment_button.h"
#include "http/webroute_equipment_buttons.h"
#include "http/webroute_equipment_devices.h"
#include "http/webroute_equipment_setpoints.h"
#include "http/webroute_equipment_version.h"
#include "http/webroute_version.h"
#include "http/websocket_equipment.h"
#include "http/websocket_equipment_stats.h"

// Core — MQTT, serial, protocol
#include "mqtt/mqtt_integration.h"
#include "protocol/message_generator_registry.h"
#include "protocol/protocol_thread.h"
#include "serial/port_types/network_serial_port_impl.h"
#include "serial/port_types/physical_serial_port_impl.h"
#include "serial/serial_initialise.h"
#include "serial/serial_port.h"

// Jandy protocol
#include "jandy/devices/command_dispatcher.h"
#include "jandy/options/options_jandy.h"
#include "jandy/jandy.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/iaq/iaq_message_control_data_response.h"
#include "jandy/protocol/jandy_protocol_registration.h"

// Pentair protocol
#include "pentair/options/options_pentair.h"
#include "pentair/pentair.h"

#include "aqualink-automate.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

int main(int argc, char* argv[])
{
	int return_value = EXIT_FAILURE;

	try
	{

		//---------------------------------------------------------------------
		// LOGGING
		//---------------------------------------------------------------------

		Logging::SeverityFiltering::SetGlobalFilterLevel(Severity::Info);
		Logging::Initialise();

		//---------------------------------------------------------------------
		// PROFILING
		//---------------------------------------------------------------------

		auto& profiler = Factory::ProfilerFactory::Instance();
		auto& profiler_units = Factory::ProfilingUnitFactory::Instance();

		Profiling::RegisterAvailableProfilers(profiler, profiler_units);
		profiler.Get()->StartProfiling();
		profiler.Get()->AppInfo(Version::VersionDetails());

		//---------------------------------------------------------------------
		// OPTIONS
		//---------------------------------------------------------------------

		LogInfo(Channel::Options, "Configuring application options");

		Options::Settings settings;

		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("main -> options_parsing", std::source_location::current());

			LogDebug(Channel::Options, "Parsing application options provided via command line");

			auto processed_options = Options::Initialise()
				| Add(Options::App::OptionsProcessor{})
				| Add(Options::Developer::OptionsProcessor{})
				| Add(Options::Equipment::OptionsProcessor{})
				| Add(Options::Mqtt::OptionsProcessor{})
				| Add(Options::Serial::OptionsProcessor{})
				| Add(Options::Web::OptionsProcessor{})
				| Add(Jandy::Options::OptionsProcessor{})
				| Add(Pentair::Options::OptionsProcessor{})
				| Parse(argc, argv)
				| Validate()
				| CheckHelpAndVersion()
				| Process(
					Options::App::OptionsProcessor{},
					Options::Developer::OptionsProcessor{},
					Options::Equipment::OptionsProcessor{},
					Options::Mqtt::OptionsProcessor{},
					Options::Serial::OptionsProcessor{},
					Options::Web::OptionsProcessor{},
					Jandy::Options::OptionsProcessor{},
					Pentair::Options::OptionsProcessor{})
				| Finalise();

			if (!processed_options)
			{
				LogFatal(Channel::Options, std::format("Failed to process application options: {}", magic_enum::enum_name(processed_options.error())));
				return EXIT_FAILURE;
			}

			settings = processed_options.value();
		}

		//---------------------------------------------------------------------
		// IO CONTEXT
		//---------------------------------------------------------------------

		boost::asio::io_context io_context;

		//---------------------------------------------------------------------
		// INFORMATION DISTRIBUTION HUBS
		//---------------------------------------------------------------------

		Kernel::HubLocator hub_locator;

		auto data_hub = std::make_shared<Kernel::DataHub>();
		auto equipment_hub = std::make_shared<Kernel::EquipmentHub>();
		auto preferences_hub = std::make_shared<Kernel::PreferencesHub>();
		auto statistics_hub = std::make_shared<Kernel::StatisticsHub>();

		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("main -> hub_initialisation", std::source_location::current());
			hub_locator.Register(data_hub).Register(equipment_hub).Register(preferences_hub).Register(statistics_hub);

			auto command_dispatcher = std::make_shared<Devices::CommandDispatcher>(data_hub, equipment_hub);
			hub_locator.Register<Interfaces::ICommandDispatcher>(command_dispatcher);
		}

		//---------------------------------------------------------------------
		// EQUIPMENT CONFIGURATION (from CLI options)
		//---------------------------------------------------------------------

		{
			auto equipment_settings_result = settings.Get<Options::Equipment::EquipmentSettings>();
			if (equipment_settings_result)
			{
				const auto& equipment_settings = equipment_settings_result.value().get();

				if (equipment_settings.pool_configuration_is_user_specified)
				{
					data_hub->ApplyPoolConfiguration(equipment_settings.pool_configuration, Kernel::ConfigurationSource::UserSpecified);

					LogInfo(Channel::Equipment, std::format("Pool configuration set from CLI: {}", magic_enum::enum_name(equipment_settings.pool_configuration)));
				}
			}
		}

		//---------------------------------------------------------------------
		// SUPPORTED EQUIPMENT TYPES
		//---------------------------------------------------------------------

		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("main -> equipment_initialisation", std::source_location::current());
			Jandy::Initialise(hub_locator);
			Pentair::Initialise(hub_locator);
		}

		//---------------------------------------------------------------------
		// SERIAL PORT
		//---------------------------------------------------------------------

		std::shared_ptr<AqualinkAutomate::Serial::SerialPort> serial_port;

		{
			auto serial_settings_result = settings.Get<Options::Serial::SerialSettings>();
			auto developer_settings_result = settings.Get<Options::Developer::DeveloperSettings>();

			if (!serial_settings_result)
			{
				LogFatal(Channel::Serial, std::format("Serial settings not available: {}", serial_settings_result.error()));
				return EXIT_FAILURE;
			}
			else if (!developer_settings_result)
			{
				LogFatal(Channel::Main, std::format("Developer settings not available: {}", developer_settings_result.error()));
				return EXIT_FAILURE;
			}
			else
			{
				const auto& developer_settings = developer_settings_result.value().get();
				const auto& serial_settings = serial_settings_result.value().get();

				LogDebug(Channel::Serial, std::format("Serial settings: port='{}', remote='{}', baud={}, rfc2217={}, rawtcp={}",
					serial_settings.serial_port,
					serial_settings.remote_serial_port,
					serial_settings.baud_rate,
					serial_settings.use_rfc2217,
					serial_settings.use_rawtcp));

				auto executor = io_context.get_executor();

				if ((developer_settings.dev_mode_enabled) && (!developer_settings.replay_file.empty()))
				{
					LogInfo(Channel::Main, "Enabling developer mode");

					auto serial_port_impl = std::make_unique<AqualinkAutomate::Developer::MockSerialPortImpl>();
					serial_port = std::make_shared<AqualinkAutomate::Serial::SerialPort>(std::move(serial_port_impl), hub_locator);
					serial_port->open(developer_settings.replay_file);
				}
				else if (serial_settings.UsingPhysicalSerialPort())
				{
					LogDebug(Channel::Main, std::format("Using a physical serial port ({})", serial_settings.serial_port));

					std::unique_ptr<Interfaces::ISerialPortImpl> serial_port_impl = std::make_unique<AqualinkAutomate::Serial::PortTypes::PhysicalSerialPortImpl>(executor);

					if (!developer_settings.recording_file.empty())
					{
						LogInfo(Channel::Serial, std::format("Enabling serial recording to: {}", developer_settings.recording_file));
						serial_port_impl = std::make_unique<AqualinkAutomate::Developer::RecordingSerialPortImpl>(std::move(serial_port_impl), developer_settings.recording_file);
					}

					serial_port = std::make_shared<AqualinkAutomate::Serial::SerialPort>(std::move(serial_port_impl), hub_locator);

					if (!AqualinkAutomate::Serial::Initialise(settings, serial_port))
					{
						LogFatal(Channel::Serial, std::format("Failed to initialise serial port '{}'; cannot continue", serial_settings.serial_port));
						return EXIT_FAILURE;
					}
				}
				else if (serial_settings.UsingRemoteSerialPort())
				{
					LogDebug(Channel::Main, std::format("Using a remote serial port ({})", serial_settings.remote_serial_port));

					std::unique_ptr<Interfaces::ISerialPortImpl> serial_port_impl = std::make_unique<AqualinkAutomate::Serial::PortTypes::NetworkSerialPortImpl>(executor);

					if (!developer_settings.recording_file.empty())
					{
						LogInfo(Channel::Serial, std::format("Enabling serial recording to: {}", developer_settings.recording_file));
						serial_port_impl = std::make_unique<AqualinkAutomate::Developer::RecordingSerialPortImpl>(std::move(serial_port_impl), developer_settings.recording_file);
					}

					serial_port = std::make_shared<AqualinkAutomate::Serial::SerialPort>(std::move(serial_port_impl), hub_locator);

					if (!AqualinkAutomate::Serial::Initialise(settings, serial_port))
					{
						LogFatal(Channel::Serial, std::format("Failed to initialise remote serial port '{}'; cannot continue", serial_settings.remote_serial_port));
						return EXIT_FAILURE;
					}
				}
				else
				{
					LogFatal(Channel::Serial, "No serial port configured (physical, remote, or developer mode)");
					return EXIT_FAILURE;
				}
			}
		}

		//---------------------------------------------------------------------
		// PROTOCOL HANDLER
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Starting AqualinkAutomate::ProtocolHandler...");

		// Register protocol-specific message generators before starting the handler
		Jandy::Protocol::RegisterMessageGenerator();

		auto protocol_task = std::make_shared<AqualinkAutomate::Protocol::ProtocolTask>(serial_port, statistics_hub);
		protocol_task->ConnectWriteSignal<Messages::JandyMessage_Ack>();
		protocol_task->ConnectWriteSignal<Messages::IAQMessage_ControlDataResponse>();

		//---------------------------------------------------------------------
		// SUPPORTED EQUIPMENT
		//---------------------------------------------------------------------

		Jandy::Configure(hub_locator, settings);
		Pentair::Configure(hub_locator, settings);

		//---------------------------------------------------------------------
		// WEB SERVER
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Starting AqualinkAutomate::HttpServer...");

		AqualinkAutomate::Developer::FirewallUtils::CheckAndConfigureExceptions();

		auto web_settings_result = settings.Get<Options::Web::WebSettings>();

		std::unique_ptr<HTTP::HttpServer> http_server;
		std::unique_ptr<HTTP::HttpServer> https_server;
		boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tls_server);

		if (web_settings_result)
		{
			const auto& web_settings = web_settings_result.value().get();

			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Diagnostics_Logging>());
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Equipment>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Equipment_Button>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Equipment_Buttons>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Equipment_Devices>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Equipment_Setpoints>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Equipment_Version>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Version>());

			HTTP::Routing::Add(std::make_unique<HTTP::WebSocket_Equipment>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebSocket_Equipment_Stats>(hub_locator));

			HTTP::Routing::StaticHandler(HTTP::StaticFileHandler("/", web_settings.doc_root));

			if (web_settings.https_server_is_enabled)
			{
				ssl_context.set_options(
					boost::asio::ssl::context::default_workarounds |
					boost::asio::ssl::context::no_sslv2 |
					boost::asio::ssl::context::no_sslv3 |
					boost::asio::ssl::context::no_tlsv1 |
					boost::asio::ssl::context::no_tlsv1_1 |
					boost::asio::ssl::context::single_dh_use |
					static_cast<long>(SSL_OP_CIPHER_SERVER_PREFERENCE)
				);

				Certificates::LoadSslCertificates(web_settings, ssl_context);

				auto endpoint = boost::asio::ip::tcp::endpoint{ boost::asio::ip::make_address(web_settings.bind_address), web_settings.https_port };
				https_server = std::make_unique<HTTP::HttpServer>(io_context, endpoint, std::ref(ssl_context));
				https_server->Start();
			}

			if (web_settings.http_server_is_enabled)
			{
				auto endpoint = boost::asio::ip::tcp::endpoint{ boost::asio::ip::make_address(web_settings.bind_address), web_settings.http_port };
				http_server = std::make_unique<HTTP::HttpServer>(io_context, endpoint);
				http_server->Start();
			}
		}

		//---------------------------------------------------------------------
		// MQTT SERVICE
		//---------------------------------------------------------------------

		std::shared_ptr<AqualinkAutomate::Mqtt::MqttIntegration> mqtt_integration;

		auto mqtt_settings_result = settings.Get<Options::Mqtt::MqttSettings>();
		if (mqtt_settings_result)
		{
			const auto& mqtt_settings = mqtt_settings_result.value().get();

			if (mqtt_settings.enabled)
			{
				LogInfo(Channel::Main, "Starting AqualinkAutomate::MqttService...");

				mqtt_integration = std::make_shared<AqualinkAutomate::Mqtt::MqttIntegration>(io_context, mqtt_settings);
				mqtt_integration->ConnectHubs(data_hub, equipment_hub, statistics_hub);
				mqtt_integration->Start();

				LogInfo(Channel::Main, std::format("MQTT service enabled, publishing to {}:{}", mqtt_settings.broker_host, mqtt_settings.broker_port));
			}
			else
			{
				LogInfo(Channel::Main, "MQTT service is disabled");
			}
		}
		else
		{
			LogInfo(Channel::Main, "MQTT service is disabled");
		}

		//---------------------------------------------------------------------
		// FRAME LOOP
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Starting AqualinkAutomate...");
		profiler.Get()->Message("Application starting", static_cast<uint32_t>(Profiling::UnitColours::Green));

		using clock = std::chrono::steady_clock;
		constexpr auto FRAME_PERIOD = std::chrono::milliseconds(1);

		bool shutdown = false;
		boost::asio::signal_set shutdown_signals(io_context, SIGINT, SIGTERM);
		shutdown_signals.async_wait([&shutdown](const boost::system::error_code&, int signum)
		{
			LogInfo(Channel::Main, std::format("Received shutdown signal ({})", signum));
			shutdown = true;
		});

		profiler.Get()->EmitFrameMark("MainLoop");

		while (!shutdown)
		{
			auto frame_start = clock::now();

			// Process any pending Asio handlers (signal_set, etc.)
			io_context.poll();

			// Advance subsystems
			bool had_work = protocol_task->Poll();

			if (http_server)
			{
				http_server->Poll();
			}

			if (https_server)
			{
				https_server->Poll();
			}

			if (mqtt_integration)
			{
				mqtt_integration->Poll();
			}

			// Adaptive sleep: skip the sleep when the protocol task had
			// work to do so that the next read/process/write cycle starts
			// immediately — giving near-zero response latency under load.
			// When idle, sleep to avoid busy-spinning.
			if (!had_work)
			{
				std::this_thread::sleep_until(frame_start + FRAME_PERIOD);
			}

			profiler.Get()->EmitFrameMark("MainLoop");
		}

		//---------------------------------------------------------------------
		// STOP APPLICATION
		//---------------------------------------------------------------------

		LogInfo(Channel::Main, "Stopping AqualinkAutomate...");
		profiler.Get()->Message("Application shutting down", static_cast<uint32_t>(Profiling::UnitColours::Orange));

		// 1. Cancel serial port to unblock the protocol task read loop.
		if (serial_port && serial_port->is_open())
		{
			boost::system::error_code cancel_ec;
			serial_port->cancel(cancel_ec);
		}

		// 2. Release protocol task resources.
		protocol_task.reset();

		// 3. Stop MQTT integration
		if (mqtt_integration)
		{
			LogInfo(Channel::Main, "Stopping MQTT service...");
			mqtt_integration->Stop();
			mqtt_integration.reset();
		}

		// 4. Stop HTTP servers
		if (https_server)
		{
			https_server->Stop();
			https_server.reset();
		}
		if (http_server)
		{
			http_server->Stop();
			http_server.reset();
		}

		// 5. Clear HTTP routing tables
		LogInfo(Channel::Main, "Clearing HTTP routing tables...");
		HTTP::Routing::Clear();

		// 6. Clear message generator registry
		LogInfo(Channel::Main, "Clearing message generator registry...");
		Protocol::MessageGeneratorRegistry::Instance().Clear();

		// 7. Stop profiling last
		profiler.Get()->StopProfiling();

		return_value = EXIT_SUCCESS;
	}
	catch (const Exceptions::OptionsHelpOrVersion&)
	{
		return_value = EXIT_SUCCESS;
	}
	catch (const Exceptions::OptionParsingFailed&)
	{
		return_value = EXIT_SUCCESS;
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

		for (const auto& frame : st)
		{
			LogDebug(Channel::Main, std::format("{}, {}({})", frame.description(), frame.source_file().empty() ? "Unknown File" : frame.source_file(), frame.source_line()));
		}
	}
	catch (const boost::system::system_error& err)
	{
		LogFatal(Channel::Main, std::format("Unknown exception occurred...terminating!  Message: {}", err.what()));
	}
	catch (const std::exception& err)
	{
		LogFatal(Channel::Main, std::format("Unknown exception occurred...terminating!  Message: {}", err.what()));

		const auto trace = std::stacktrace::current();
		for (const auto& frame : trace)
		{
			LogDebug(Channel::Main, std::format("{}, {}({})", frame.description(), frame.source_file().empty() ? "Unknown File" : frame.source_file(), frame.source_line()));
		}

	}

	return return_value;
}
