#include <boost/test/unit_test.hpp>

#include <boost/program_options.hpp>

#include "options/options.h"
#include "jandy/options/options_jandy.h"
#include "pentair/options/options_pentair.h"

using namespace AqualinkAutomate;
namespace po = boost::program_options;

namespace
{
	/// Runs the full options pipeline matching main's exact pattern:
	///   Initialise | Add(all) | Parse | Validate | Process(all) | Finalise
	///
	/// Note: CheckHelpAndVersion is omitted because it writes to stdout/throws
	/// when --help or --version is present. Since we never pass those in tests,
	/// omitting it is equivalent to including it.
	auto RunFullPipeline(const std::vector<const char*>& args)
		-> std::expected<Options::Settings, AqualinkAutomate::ErrorCodes::Options_ErrorCodes>
	{
		return Options::Initialise()
			| Options::Add(Options::App::OptionsProcessor{})
			| Options::Add(Options::Developer::OptionsProcessor{})
			| Options::Add(Options::Mqtt::OptionsProcessor{})
			| Options::Add(Options::Serial::OptionsProcessor{})
			| Options::Add(Options::Web::OptionsProcessor{})
			| Options::Add(Jandy::Options::OptionsProcessor{})
			| Options::Add(Pentair::Options::OptionsProcessor{})
			| Options::Parse(static_cast<int>(args.size()), const_cast<char**>(args.data()))
			| Options::Validate()
			| Options::Process(
				Options::App::OptionsProcessor{},
				Options::Developer::OptionsProcessor{},
				Options::Mqtt::OptionsProcessor{},
				Options::Serial::OptionsProcessor{},
				Options::Web::OptionsProcessor{},
				Jandy::Options::OptionsProcessor{},
				Pentair::Options::OptionsProcessor{})
			| Options::Finalise();
	}
}

BOOST_AUTO_TEST_SUITE(TestSuite_OptionsPipeline)

//=============================================================================
// FULL PIPELINE: All settings areas are present
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_Pipeline_AllSettingsAreasPopulated)
{
	auto result = RunFullPipeline({ "program" });
	BOOST_REQUIRE(result.has_value());

	const auto& settings = result.value();

	BOOST_CHECK(settings.Has("Application"));
	BOOST_CHECK(settings.Has("Developer"));
	BOOST_CHECK(settings.Has("MQTT"));
	BOOST_CHECK(settings.Has("Serial"));
	BOOST_CHECK(settings.Has("Web"));
	BOOST_CHECK(settings.Has("Jandy"));
	BOOST_CHECK(settings.Has("Pentair"));
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_AllSettingsAreasRetrievable)
{
	auto result = RunFullPipeline({ "program" });
	BOOST_REQUIRE(result.has_value());

	const auto& settings = result.value();

	BOOST_CHECK(settings.Get<Options::App::AppSettings>().has_value());
	BOOST_CHECK(settings.Get<Options::Developer::DeveloperSettings>().has_value());
	BOOST_CHECK(settings.Get<Options::Mqtt::MqttSettings>().has_value());
	BOOST_CHECK(settings.Get<Options::Serial::SerialSettings>().has_value());
	BOOST_CHECK(settings.Get<Options::Web::WebSettings>().has_value());
	BOOST_CHECK(settings.Get<Jandy::Options::JandySettings>().has_value());
	BOOST_CHECK(settings.Get<Pentair::Options::PentairSettings>().has_value());
}

//=============================================================================
// FULL PIPELINE: Default values propagate correctly
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_Pipeline_WebDefaults)
{
	auto result = RunFullPipeline({ "program" });
	BOOST_REQUIRE(result.has_value());

	auto web = result.value().Get<Options::Web::WebSettings>();
	BOOST_REQUIRE(web.has_value());

	const auto& s = web.value().get();
	BOOST_CHECK(s.http_server_is_enabled);
	BOOST_CHECK(s.https_server_is_enabled);
	BOOST_CHECK(!s.http_content_is_disabled);
	BOOST_CHECK_EQUAL(s.bind_address, "0.0.0.0");
	BOOST_CHECK_EQUAL(s.http_port, 80);
	BOOST_CHECK_EQUAL(s.https_port, 443);
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_SerialDefaults)
{
	auto result = RunFullPipeline({ "program" });
	BOOST_REQUIRE(result.has_value());

	auto serial = result.value().Get<Options::Serial::SerialSettings>();
	BOOST_REQUIRE(serial.has_value());

	const auto& s = serial.value().get();
	BOOST_CHECK_EQUAL(s.serial_port, Application::SERIAL_PORT);
	BOOST_CHECK_EQUAL(s.baud_rate, 9600);
	BOOST_CHECK(s.remote_serial_port.empty());
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_MqttDefaults)
{
	auto result = RunFullPipeline({ "program" });
	BOOST_REQUIRE(result.has_value());

	auto mqtt = result.value().Get<Options::Mqtt::MqttSettings>();
	BOOST_REQUIRE(mqtt.has_value());

	const auto& s = mqtt.value().get();
	BOOST_CHECK(!s.enabled);
	BOOST_CHECK_EQUAL(s.broker_host, "localhost");
	BOOST_CHECK_EQUAL(s.broker_port, 1883);
	BOOST_CHECK(!s.use_tls);
	BOOST_CHECK_EQUAL(s.topic_prefix, "aqualink");
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_DeveloperDefaults)
{
	auto result = RunFullPipeline({ "program" });
	BOOST_REQUIRE(result.has_value());

	auto dev = result.value().Get<Options::Developer::DeveloperSettings>();
	BOOST_REQUIRE(dev.has_value());

	const auto& s = dev.value().get();
	BOOST_CHECK(!s.dev_mode_enabled);
	BOOST_CHECK(s.replay_file.empty());
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_JandyDefaults)
{
	auto result = RunFullPipeline({ "program" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());

	const auto& s = jandy.value().get();
	BOOST_CHECK(!s.disable_emulation);
	BOOST_CHECK(s.emulated_devices.empty());
}

//=============================================================================
// FULL PIPELINE: Explicit args propagate across processors
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_Pipeline_WebArgs_DisableHttps)
{
	auto result = RunFullPipeline({ "program", "--disable-https" });
	BOOST_REQUIRE(result.has_value());

	auto web = result.value().Get<Options::Web::WebSettings>();
	BOOST_REQUIRE(web.has_value());

	BOOST_CHECK(!web.value().get().https_server_is_enabled);
	BOOST_CHECK(web.value().get().http_server_is_enabled);
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_WebArgs_DisableHttp)
{
	auto result = RunFullPipeline({ "program", "--disable-http" });
	BOOST_REQUIRE(result.has_value());

	auto web = result.value().Get<Options::Web::WebSettings>();
	BOOST_REQUIRE(web.has_value());

	BOOST_CHECK(web.value().get().https_server_is_enabled);
	BOOST_CHECK(!web.value().get().http_server_is_enabled);
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_WebArgs_CustomPorts)
{
	auto result = RunFullPipeline({ "program", "--http-port=8080", "--https-port=8443" });
	BOOST_REQUIRE(result.has_value());

	auto web = result.value().Get<Options::Web::WebSettings>();
	BOOST_REQUIRE(web.has_value());

	BOOST_CHECK_EQUAL(web.value().get().http_port, 8080);
	BOOST_CHECK_EQUAL(web.value().get().https_port, 8443);
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_SerialArgs_CustomPort)
{
	auto result = RunFullPipeline({ "program", "--serial-port=COM5" });
	BOOST_REQUIRE(result.has_value());

	auto serial = result.value().Get<Options::Serial::SerialSettings>();
	BOOST_REQUIRE(serial.has_value());

	BOOST_CHECK_EQUAL(serial.value().get().serial_port, "COM5");
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_SerialArgs_CustomBaudRate)
{
	auto result = RunFullPipeline({ "program", "--baudrate=19200" });
	BOOST_REQUIRE(result.has_value());

	auto serial = result.value().Get<Options::Serial::SerialSettings>();
	BOOST_REQUIRE(serial.has_value());

	BOOST_CHECK_EQUAL(serial.value().get().baud_rate, 19200);
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_MqttArgs_EnableMqtt)
{
	auto result = RunFullPipeline({ "program", "--mqtt" });
	BOOST_REQUIRE(result.has_value());

	auto mqtt = result.value().Get<Options::Mqtt::MqttSettings>();
	BOOST_REQUIRE(mqtt.has_value());

	BOOST_CHECK(mqtt.value().get().enabled);
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_MqttArgs_BrokerConfig)
{
	auto result = RunFullPipeline({ "program", "--mqtt", "--mqtt-host=broker.local", "--mqtt-port=1884" });
	BOOST_REQUIRE(result.has_value());

	auto mqtt = result.value().Get<Options::Mqtt::MqttSettings>();
	BOOST_REQUIRE(mqtt.has_value());

	const auto& s = mqtt.value().get();
	BOOST_CHECK(s.enabled);
	BOOST_CHECK_EQUAL(s.broker_host, "broker.local");
	BOOST_CHECK_EQUAL(s.broker_port, 1884);
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_MqttArgs_TopicPrefix)
{
	auto result = RunFullPipeline({ "program", "--mqtt", "--mqtt-prefix=home/pool" });
	BOOST_REQUIRE(result.has_value());

	auto mqtt = result.value().Get<Options::Mqtt::MqttSettings>();
	BOOST_REQUIRE(mqtt.has_value());

	BOOST_CHECK_EQUAL(mqtt.value().get().topic_prefix, "home/pool");
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_MqttArgs_TlsAutoPort)
{
	// Enabling TLS without explicit port should switch to 8883
	auto result = RunFullPipeline({ "program", "--mqtt", "--mqtt-tls" });
	BOOST_REQUIRE(result.has_value());

	auto mqtt = result.value().Get<Options::Mqtt::MqttSettings>();
	BOOST_REQUIRE(mqtt.has_value());

	BOOST_CHECK(mqtt.value().get().use_tls);
	BOOST_CHECK_EQUAL(mqtt.value().get().broker_port, 8883);
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_MqttArgs_TlsExplicitPort)
{
	// Enabling TLS with explicit port should keep that port
	auto result = RunFullPipeline({ "program", "--mqtt", "--mqtt-tls", "--mqtt-port=9883" });
	BOOST_REQUIRE(result.has_value());

	auto mqtt = result.value().Get<Options::Mqtt::MqttSettings>();
	BOOST_REQUIRE(mqtt.has_value());

	BOOST_CHECK(mqtt.value().get().use_tls);
	BOOST_CHECK_EQUAL(mqtt.value().get().broker_port, 9883);
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_MqttArgs_PublishIntervals)
{
	auto result = RunFullPipeline({ "program", "--mqtt", "--mqtt-status-interval=15", "--mqtt-stats-interval=120" });
	BOOST_REQUIRE(result.has_value());

	auto mqtt = result.value().Get<Options::Mqtt::MqttSettings>();
	BOOST_REQUIRE(mqtt.has_value());

	BOOST_CHECK_EQUAL(mqtt.value().get().status_publish_interval.count(), 15);
	BOOST_CHECK_EQUAL(mqtt.value().get().statistics_publish_interval.count(), 120);
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_DeveloperArgs_DevMode)
{
	auto result = RunFullPipeline({ "program", "--dev-mode" });
	BOOST_REQUIRE(result.has_value());

	auto dev = result.value().Get<Options::Developer::DeveloperSettings>();
	BOOST_REQUIRE(dev.has_value());

	BOOST_CHECK(dev.value().get().dev_mode_enabled);
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_JandyArgs_DisableEmulation)
{
	auto result = RunFullPipeline({ "program", "--disable-emulation" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());

	BOOST_CHECK(jandy.value().get().disable_emulation);
}

//=============================================================================
// FULL PIPELINE: Cross-processor isolation
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_Pipeline_WebArgsDoNotAffectMqtt)
{
	auto result = RunFullPipeline({ "program", "--disable-https", "--disable-http" });
	BOOST_REQUIRE(result.has_value());

	// Web settings should be modified
	auto web = result.value().Get<Options::Web::WebSettings>();
	BOOST_REQUIRE(web.has_value());
	BOOST_CHECK(!web.value().get().https_server_is_enabled);
	BOOST_CHECK(!web.value().get().http_server_is_enabled);

	// MQTT settings should be unaffected
	auto mqtt = result.value().Get<Options::Mqtt::MqttSettings>();
	BOOST_REQUIRE(mqtt.has_value());
	BOOST_CHECK(!mqtt.value().get().enabled);
	BOOST_CHECK_EQUAL(mqtt.value().get().broker_host, "localhost");
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_MqttArgsDoNotAffectWeb)
{
	auto result = RunFullPipeline({ "program", "--mqtt", "--mqtt-host=broker.local" });
	BOOST_REQUIRE(result.has_value());

	// MQTT settings should be modified
	auto mqtt = result.value().Get<Options::Mqtt::MqttSettings>();
	BOOST_REQUIRE(mqtt.has_value());
	BOOST_CHECK(mqtt.value().get().enabled);

	// Web settings should be unaffected
	auto web = result.value().Get<Options::Web::WebSettings>();
	BOOST_REQUIRE(web.has_value());
	BOOST_CHECK(web.value().get().https_server_is_enabled);
	BOOST_CHECK(web.value().get().http_server_is_enabled);
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_MultipleProcessorsSimultaneously)
{
	auto result = RunFullPipeline({
		"program",
		"--disable-https",
		"--mqtt", "--mqtt-host=pool-broker",
		"--serial-port=COM3", "--baudrate=19200",
		"--disable-emulation"
	});
	BOOST_REQUIRE(result.has_value());

	// Web
	auto web = result.value().Get<Options::Web::WebSettings>();
	BOOST_REQUIRE(web.has_value());
	BOOST_CHECK(!web.value().get().https_server_is_enabled);
	BOOST_CHECK(web.value().get().http_server_is_enabled);

	// MQTT
	auto mqtt = result.value().Get<Options::Mqtt::MqttSettings>();
	BOOST_REQUIRE(mqtt.has_value());
	BOOST_CHECK(mqtt.value().get().enabled);
	BOOST_CHECK_EQUAL(mqtt.value().get().broker_host, "pool-broker");

	// Serial
	auto serial = result.value().Get<Options::Serial::SerialSettings>();
	BOOST_REQUIRE(serial.has_value());
	BOOST_CHECK_EQUAL(serial.value().get().serial_port, "COM3");
	BOOST_CHECK_EQUAL(serial.value().get().baud_rate, 19200);

	// Jandy
	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());
	BOOST_CHECK(jandy.value().get().disable_emulation);
}

//=============================================================================
// VALIDATION: Conflicting options cause pipeline failure
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_Pipeline_Validation_ConflictingWebOptions)
{
	auto result = RunFullPipeline({ "program", "--disable-https", "--https-port=8443" });
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_Validation_ConflictingSerialOptions)
{
	// serial-port has a default value; remote-serial-port does not.
	// Passing --remote-serial-port explicitly while --serial-port is
	// defaulted should NOT conflict (defaulted options are excluded).
	auto result = RunFullPipeline({ "program", "--remote-serial-port=192.168.1.100:2000" });
	BOOST_CHECK(result.has_value());
}

BOOST_AUTO_TEST_CASE(Test_Pipeline_Validation_ExplicitConflictingSerialOptions)
{
	// Both explicitly provided = conflict
	auto result = RunFullPipeline({ "program", "--serial-port=COM5", "--remote-serial-port=192.168.1.100:2000" });
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END()
