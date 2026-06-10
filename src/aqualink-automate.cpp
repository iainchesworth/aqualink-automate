// Standard library
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iostream>
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
#include "interfaces/irecordingcontroller.h"
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
#include "http/webroute_auth_check.h"
#include "http/webroute_diagnostics_actualdevices.h"
#include "http/webroute_diagnostics_devices.h"
#include "http/webroute_diagnostics_logging.h"
#include "http/webroute_diagnostics_options.h"
#include "http/webroute_diagnostics_recording.h"
#include "http/webroute_equipment.h"
#include "http/webroute_equipment_button.h"
#include "http/webroute_equipment_buttons.h"
#include "http/webroute_equipment_devices.h"
#include "http/webroute_equipment_setpoints.h"
#include "http/webroute_equipment_version.h"
#include "http/webroute_metrics.h"
#include "http/webroute_version.h"
#include "http/websocket_equipment.h"
#include "http/websocket_equipment_stats.h"

// Core — alerting (fault detection)
#include "alerting/alert_condition.h"
#include "alerting/alert_monitor.h"
#include "alerting/webhook_sink.h"

// Core — history (time-series persistence)
#include "history/history_service.h"
#include "http/webroute_history.h"

// Core — preferences (user/admin settings)
#include "preferences/preferences_service.h"
#include "http/webroute_preferences.h"

// Core — equipment cache (instant dashboard on restart)
#include "equipment_cache/equipment_cache_service.h"

// Core — scheduling (time-based automation)
#include "scheduling/scheduler_service.h"
#include "http/webroute_schedules.h"

// Core — MQTT, serial, protocol
#include "mqtt/mqtt_hub.h"
#include "mqtt/mqtt_client.h"
#include "mqtt/mqtt_integration.h"
#include "protocol/message_generator_registry.h"
#include "protocol/protocol_thread.h"
#include "serial/port_types/network_serial_port_impl.h"
#include "serial/port_types/physical_serial_port_impl.h"
#include "serial/serial_initialise.h"
#include "serial/serial_port.h"

// Shared device capabilities
#include "devices/capabilities/restartable.h"

// Jandy protocol
#include "jandy/devices/command_dispatcher.h"
#include "jandy/options/options_jandy.h"
#include "jandy/jandy.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/iaq/iaq_message_control_data_response.h"
#include "jandy/protocol/jandy_protocol_registration.h"

// Pentair protocol
#include "pentair/options/options_pentair.h"
#include "pentair/protocol/pentair_protocol_registration.h"
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
				| Add(Options::Alerting::OptionsProcessor{})
				| Add(Options::App::OptionsProcessor{})
				| Add(Options::Developer::OptionsProcessor{})
				| Add(Options::Equipment::OptionsProcessor{})
				| Add(Options::History::OptionsProcessor{})
				| Add(Options::Mqtt::OptionsProcessor{})
				| Add(Options::Preferences::OptionsProcessor{})
				| Add(Options::Scheduling::OptionsProcessor{})
				| Add(Options::Serial::OptionsProcessor{})
				| Add(Options::Web::OptionsProcessor{})
				| Add(Jandy::Options::OptionsProcessor{})
				| Add(Pentair::Options::OptionsProcessor{})
				| Parse(argc, argv)
				| ParseConfigFile()
				// CheckHelpAndVersion MUST run before Validate: --help/--version
				// short-circuit (they throw OptionsHelpOrVersion, caught in main as
				// EXIT_SUCCESS) and must not be blocked by a conflict/dependency
				// validation failure on the rest of the command line.
				| CheckHelpAndVersion()
				| Validate()
				| Process(
					Options::Alerting::OptionsProcessor{},
					Options::App::OptionsProcessor{},
					Options::Developer::OptionsProcessor{},
					Options::Equipment::OptionsProcessor{},
					Options::History::OptionsProcessor{},
					Options::Mqtt::OptionsProcessor{},
					Options::Preferences::OptionsProcessor{},
					Options::Scheduling::OptionsProcessor{},
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

				// Always place the runtime-controllable recording decorator in the
				// PRODUCTION serial chain (physical AND remote).  It defaults to OFF
				// (a transparent pass-through) and is toggled at runtime via the
				// /api/diagnostics/recording route, which resolves it from the
				// HubLocator as an IRecordingController.  If `--record-serial <file>`
				// was supplied it starts recording at boot (legacy behaviour).  The
				// decorator is owned by the SerialPort (lifetime = whole app); the
				// HubLocator holds a non-owning handle (null deleter).
				auto install_recording_decorator =
					[&hub_locator, &developer_settings](std::unique_ptr<Interfaces::ISerialPortImpl> base) -> std::unique_ptr<Interfaces::ISerialPortImpl>
					{
						std::unique_ptr<AqualinkAutomate::Developer::RecordingSerialPortImpl> recorder;
						if (!developer_settings.recording_file.empty())
						{
							LogInfo(Channel::Serial, std::format("Enabling serial recording to: {}", developer_settings.recording_file));
							recorder = std::make_unique<AqualinkAutomate::Developer::RecordingSerialPortImpl>(std::move(base), developer_settings.recording_file);
						}
						else
						{
							recorder = std::make_unique<AqualinkAutomate::Developer::RecordingSerialPortImpl>(std::move(base));
						}

						// Register a non-owning handle so the diagnostics route can
						// toggle recording at runtime.  The null deleter keeps the
						// SerialPort as the sole owner.
						std::shared_ptr<Interfaces::IRecordingController> controller_handle(
							static_cast<Interfaces::IRecordingController*>(recorder.get()),
							[](Interfaces::IRecordingController*) { /* non-owning */ });
						hub_locator.Register<Interfaces::IRecordingController>(controller_handle);

						return recorder;
					};

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

					serial_port_impl = install_recording_decorator(std::move(serial_port_impl));

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

					serial_port_impl = install_recording_decorator(std::move(serial_port_impl));

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

		// Register protocol-specific message generators before starting the handler.
		// Both are registered so Jandy and Pentair traffic can be auto-detected on
		// the same serial stream (Jandy frames on DLE/STX, Pentair on a 0xFF/0xA5
		// preamble).  The registry tries generators in priority order; the Pentair
		// generator registers at priority 0 (it is non-destructive and defers on
		// non-Pentair buffers) ahead of Jandy at priority 1.
		Jandy::Protocol::RegisterMessageGenerator();
		Pentair::Protocol::RegisterMessageGenerator();

		// Capture-replay pacing: when replaying a capture file in developer mode the
		// application's frame loop (below) steps at a fixed processing period
		// (replay_frame_period_ms, scaled by replay_speed) and the protocol task
		// reads one serial chunk per frame, so frames are delivered at roughly the
		// bus's natural inter-frame rate instead of as fast as the parser will accept
		// them.  Stays unpaced (free-running, as for real ports) when not replaying
		// or when --replay-frame-period is 0.
		std::chrono::microseconds replay_frame_period{ 0 };
		if (auto developer_settings_result = settings.Get<Options::Developer::DeveloperSettings>(); developer_settings_result)
		{
			const auto& developer_settings = developer_settings_result.value().get();
			if (developer_settings.dev_mode_enabled && !developer_settings.replay_file.empty() && (developer_settings.replay_frame_period_ms > 0))
			{
				if (developer_settings.replay_speed > 0.0)
				{
					const double effective_us = (static_cast<double>(developer_settings.replay_frame_period_ms) * 1000.0) / developer_settings.replay_speed;
					replay_frame_period = std::chrono::microseconds(static_cast<std::chrono::microseconds::rep>(effective_us));
					LogInfo(Channel::Main, std::format("Capture replay pacing enabled: {:.3g} ms/frame (period {} ms, speed {:.3g})",
						static_cast<double>(replay_frame_period.count()) / 1000.0, developer_settings.replay_frame_period_ms, developer_settings.replay_speed));
				}
				else
				{
					LogWarning(Channel::Main, std::format("Ignoring invalid --replay-speed {:.3g} (must be > 0); capture replay will be unpaced", developer_settings.replay_speed));
				}
			}
		}
		const bool replay_paced = (replay_frame_period > std::chrono::microseconds::zero());

		auto protocol_task = std::make_shared<AqualinkAutomate::Protocol::ProtocolTask>(serial_port, statistics_hub, replay_paced);
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

		//---------------------------------------------------------------------
		// PREFERENCES SERVICE (user/admin settings persistence)
		//---------------------------------------------------------------------
		// Constructed first: it seeds the PreferencesHub from the effective CLI
		// values (so deployments behave identically) then loads any persisted
		// file (overriding the seed). The services below read the hub live.
		namespace Preferences = AqualinkAutomate::Preferences;

		std::shared_ptr<Preferences::PreferencesService> preferences_service;
		{
			Options::Preferences::PreferencesSettings prefs_settings;
			if (auto r = settings.Get<Options::Preferences::PreferencesSettings>(); r) { prefs_settings = r.value().get(); }

			Options::Alerting::AlertingSettings seed_alerting;
			if (auto r = settings.Get<Options::Alerting::AlertingSettings>(); r) { seed_alerting = r.value().get(); }
			Options::History::HistorySettings seed_history;
			if (auto r = settings.Get<Options::History::HistorySettings>(); r) { seed_history = r.value().get(); }

			preferences_service = std::make_shared<Preferences::PreferencesService>(hub_locator, prefs_settings);
			preferences_service->Seed(seed_alerting.salt_low_ppm, seed_alerting.comms_timeout_seconds, seed_alerting.webhook_url, seed_history.retention_days);
			preferences_service->Start();

			LogInfo(Channel::Main, prefs_settings.preferences_file.empty()
				? std::string{ "Preferences are in-memory only (no --preferences-file)" }
				: std::format("Preferences persisted to {}", prefs_settings.preferences_file));
		}

		//---------------------------------------------------------------------
		// EQUIPMENT CACHE (instant dashboard on restart)
		//---------------------------------------------------------------------
		// Loaded BEFORE the protocol task starts discovery so the DataHub already
		// holds last-known devices/config; live discovery then merges by label.
		namespace EquipmentCache = AqualinkAutomate::EquipmentCache;

		std::shared_ptr<EquipmentCache::EquipmentCacheService> equipment_cache_service;
		{
			Options::Equipment::EquipmentSettings eq_settings;
			if (auto r = settings.Get<Options::Equipment::EquipmentSettings>(); r) { eq_settings = r.value().get(); }

			equipment_cache_service = std::make_shared<EquipmentCache::EquipmentCacheService>(io_context, hub_locator, eq_settings);
			equipment_cache_service->Load();
			equipment_cache_service->Start();

			if (!eq_settings.equipment_cache_file.empty())
			{
				LogInfo(Channel::Main, std::format("Equipment cache file: {}", eq_settings.equipment_cache_file));
			}
		}

		//---------------------------------------------------------------------
		// HISTORY SERVICE (SQLite time-series persistence)
		//---------------------------------------------------------------------
		// Constructed before the routes so WebRoute_History can hold it. When
		// --history-db is unset the service stays null and the route returns 503.

		// Block-scope alias: bare `History` is otherwise ambiguous between the
		// AqualinkAutomate::History service namespace and
		// AqualinkAutomate::Options::History (both visible via using-directives).
		namespace History = AqualinkAutomate::History;

		std::shared_ptr<History::HistoryService> history_service;
		if (auto history_settings_result = settings.Get<Options::History::HistorySettings>(); history_settings_result)
		{
			const auto& history_settings = history_settings_result.value().get();
			if (!history_settings.db_path.empty())
			{
				try
				{
					history_service = std::make_shared<History::HistoryService>(io_context, hub_locator, history_settings);
					history_service->Start();
					LogInfo(Channel::Main, std::format("History service enabled (db: {})", history_settings.db_path));
				}
				catch (const std::exception& ex)
				{
					LogError(Channel::Main, std::format("History service failed to start (continuing without it): {}", ex.what()));
					history_service.reset();
				}
			}
		}

		//---------------------------------------------------------------------
		// SCHEDULER SERVICE (time-based automation)
		//---------------------------------------------------------------------
		// Block-scope alias: bare `Scheduling` is otherwise ambiguous between the
		// AqualinkAutomate::Scheduling service namespace and
		// AqualinkAutomate::Options::Scheduling.
		namespace Scheduling = AqualinkAutomate::Scheduling;

		std::shared_ptr<Scheduling::SchedulerService> scheduler_service;
		if (auto scheduling_settings_result = settings.Get<Options::Scheduling::SchedulingSettings>(); scheduling_settings_result)
		{
			const auto& scheduling_settings = scheduling_settings_result.value().get();
			if (!scheduling_settings.schedules_file.empty())
			{
				scheduler_service = std::make_shared<Scheduling::SchedulerService>(io_context, hub_locator, scheduling_settings);
				scheduler_service->Start();
				LogInfo(Channel::Main, std::format("Scheduler enabled (file: {})", scheduling_settings.schedules_file));
			}
		}

		auto web_settings_result = settings.Get<Options::Web::WebSettings>();

		std::unique_ptr<HTTP::HttpServer> http_server;
		std::unique_ptr<HTTP::HttpServer> https_server;
		boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tls_server);

		if (web_settings_result)
		{
			const auto& web_settings = web_settings_result.value().get();

			// Wire the opt-in control-plane security policy from the Web settings.
			// AuthToken unset (the default) leaves every knob disabled, so the
			// historical no-auth behaviour is preserved exactly unless the operator
			// passes --api-auth-token.
			HTTP::Routing::SecurityConfig security_config{ .AuthToken = web_settings.ApiAuthToken };

			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_AuthCheck>());
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Diagnostics_Devices>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Diagnostics_ActualDevices>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Diagnostics_Logging>());
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Diagnostics_Options>());
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Diagnostics_Recording>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Equipment>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Equipment_Button>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Equipment_Buttons>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Equipment_Devices>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Equipment_Setpoints>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Equipment_Version>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_History>(history_service));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Metrics>(hub_locator));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Preferences>(preferences_service));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Schedule>(scheduler_service));
			HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Schedules>(scheduler_service));
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
				https_server = std::make_unique<HTTP::HttpServer>(io_context, endpoint, std::ref(ssl_context), security_config);
				https_server->Start();
			}

			if (web_settings.http_server_is_enabled)
			{
				auto endpoint = boost::asio::ip::tcp::endpoint{ boost::asio::ip::make_address(web_settings.bind_address), web_settings.http_port };
				http_server = std::make_unique<HTTP::HttpServer>(io_context, endpoint, std::nullopt, security_config);
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

		// ----- Alert monitor (fault detection + alerting) --------------------
		// Block-scope alias: bare `Alerting` is otherwise ambiguous between the
		// AqualinkAutomate::Alerting subsystem and AqualinkAutomate::Options::Alerting
		// (both visible via `using namespace AqualinkAutomate`). This local
		// declaration takes lookup precedence over the using-directive names.
		namespace Alerting = AqualinkAutomate::Alerting;

		Options::Alerting::AlertingSettings alerting_settings;
		if (auto alerting_settings_result = settings.Get<Options::Alerting::AlertingSettings>(); alerting_settings_result)
		{
			alerting_settings = alerting_settings_result.value().get();
		}

		// Webhook sink — declared before the monitor so it outlives it. It reads
		// the URL LIVE from preferences (seeded from --alert-webhook-url), so the
		// webhook can be enabled/changed at runtime; an empty URL is a no-op.
		auto webhook_sink = std::make_unique<Alerting::WebhookSink>(io_context,
			[preferences_hub]() -> std::string { return preferences_hub ? preferences_hub->AlertWebhookUrl : std::string{}; });

		Alerting::AlertMonitor alert_monitor(io_context, hub_locator, alerting_settings);

		// UI sink (always): broadcast every transition to /ws/equipment clients.
		alert_monitor.AddSink([equipment_hub](const Alerting::AlertTransition& transition)
		{
			equipment_hub->AlertTransitionSignal(transition.condition, transition.raised, transition.ts, transition.detail);
		});

		// Webhook sink (always added; a no-op while the preference URL is empty).
		alert_monitor.AddSink([&webhook_sink](const Alerting::AlertTransition& transition)
		{
			webhook_sink->Post(transition);
		});

		// MQTT / Home Assistant sink: publish the consolidated alert state
		// (retained) that the HA problem binary_sensors read.  Only when MQTT is
		// active; the binary_sensor discovery itself is emitted by HA discovery.
		if (mqtt_integration)
		{
			if (auto mqtt_hub = mqtt_integration->GetMqttHub(); mqtt_hub)
			{
				if (auto mqtt_client = mqtt_hub->GetMqttClient(); mqtt_client)
				{
					const auto alert_topic = mqtt_client->BuildTopic(std::string{ Alerting::AlertStateSubtopic });

					alert_monitor.AddSink([&alert_monitor, mqtt_client, alert_topic](const Alerting::AlertTransition&)
					{
						mqtt_client->Publish(alert_topic, alert_monitor.BuildStateJson().dump(), /*retain=*/true);
					});

					// Seed the retained state so the HA entities start with a value.
					mqtt_client->Publish(alert_topic, alert_monitor.BuildStateJson().dump(), /*retain=*/true);
				}
			}
		}

		alert_monitor.Start();

		LogInfo(Channel::Main, "Starting AqualinkAutomate...");
		profiler.Get()->Message("Application starting", static_cast<uint32_t>(Profiling::UnitColours::Green));

		using clock = std::chrono::steady_clock;
		// One application frame/step.  Real-time operation steps at ~1 ms with an
		// adaptive skip (see below) for low response latency; capture replay steps
		// at the fixed processing period so playback runs at the bus's natural rate.
		const auto frame_period = replay_paced ? replay_frame_period : std::chrono::microseconds(1000);

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

			// Process any Asio handlers already pending at frame start (signal_set,
			// inbound HTTP/WS reads, MQTT socket completions, etc.).  io_context::poll
			// returns the number of handlers it ran this call.
			std::size_t handlers_run = io_context.poll();

			// Advance subsystems
			bool had_work = protocol_task->Poll();

			// Drive per-device watchdog deadline checks.
			Devices::Capabilities::Restartable::PollAll();

			// The HTTP/WS/MQTT Poll() calls queue fresh async work (notably
			// WebSocket outbound writes).  Drain it now so egress is not capped at
			// one message per frame, and so its handler count contributes to the
			// idle decision below.
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

			handlers_run += io_context.poll();

			// Any Asio handler that ran (HTTP/WebSocket/MQTT activity) counts as
			// work for pacing, so network activity keeps the loop hot rather than
			// being throttled to one frame period per message.
			had_work = had_work || (handlers_run > 0);

			// Pace the frame.  Capture replay always sleeps to the processing
			// period (a fixed step, so playback runs at the bus rate).  Real-time
			// operation sleeps only when fully idle — when the protocol task or any
			// subsystem had work we loop straight back for near-zero response
			// latency under load.
			if (replay_paced || !had_work)
			{
				std::this_thread::sleep_until(frame_start + frame_period);
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

		// Stop the alert monitor (cancels its timer + disconnects hub signals)
		// before tearing down the MQTT client its sink may reference.
		alert_monitor.Stop();

		// Flush + close the history database (also done by its destructor).
		if (history_service)
		{
			history_service->Stop();
			history_service.reset();
		}

		// Stop the scheduler timer.
		if (scheduler_service)
		{
			scheduler_service->Stop();
			scheduler_service.reset();
		}

		// Cancel the cache timer and write a final equipment snapshot.
		if (equipment_cache_service)
		{
			equipment_cache_service->Stop();
			equipment_cache_service.reset();
		}

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

		// 5. Clear HTTP routing tables (destroys the diagnostics-recording route,
		//    which cached the non-owning IRecordingController handle).
		LogInfo(Channel::Main, "Clearing HTTP routing tables...");
		HTTP::Routing::Clear();

		// 6. Drop the non-owning IRecordingController handle and tear the serial
		//    chain down deterministically.  The handle registered with the
		//    HubLocator is a null-deleter shared_ptr aliasing a SerialPort-owned
		//    RecordingSerialPortImpl, so it MUST be removed before the SerialPort
		//    (and the object it aliases) is destroyed — otherwise the HubLocator
		//    would briefly hold a dangling alias during scope-exit destruction.
		hub_locator.Unregister<Interfaces::IRecordingController>();
		serial_port.reset();

		// 7. Clear message generator registry
		LogInfo(Channel::Main, "Clearing message generator registry...");
		Protocol::MessageGeneratorRegistry::Instance().Clear();

		// 8. Stop profiling last
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
			LogDebug(Channel::Main, std::format("{}, {}({})", frame.name(), frame.source_file().empty() ? "Unknown File" : frame.source_file(), frame.source_line()));
		}
	}
	catch (const boost::system::system_error& err)
	{
		LogFatal(Channel::Main, std::format("Unknown exception occurred...terminating!  Message: {}", err.what()));
	}
	catch (const std::exception& err)
	{
		LogFatal(Channel::Main, std::format("Unknown exception occurred...terminating!  Message: {}", err.what()));

		const auto trace = boost::stacktrace::stacktrace();
		for (const auto& frame : trace)
		{
			LogDebug(Channel::Main, std::format("{}, {}({})", frame.name(), frame.source_file().empty() ? "Unknown File" : frame.source_file(), frame.source_line()));
		}

	}

	return return_value;
}
