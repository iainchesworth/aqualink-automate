#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "options/options.h"
#include "jandy/options/options_jandy.h"
#include "pentair/options/options_pentair.h"

using namespace AqualinkAutomate;

namespace
{
	/// Temporary config file helper — writes content and cleans up on destruction.
	class TempConfigFile
	{
	public:
		explicit TempConfigFile(const std::string& content, const std::string& name = "test_config.conf")
			: m_Path(std::filesystem::temp_directory_path() / name)
		{
			std::ofstream out(m_Path);
			out << content;
		}

		~TempConfigFile()
		{
			std::filesystem::remove(m_Path);
		}

		std::string path() const { return m_Path.string(); }

	private:
		std::filesystem::path m_Path;
	};

	/// Run the full pipeline with the given args (same as test_options_pipeline.cpp
	/// but using ParseConfigFile instead of Notify).
	auto RunPipelineWithConfigFile(const std::vector<const char*>& args)
		-> std::expected<Options::Settings, AqualinkAutomate::ErrorCodes::Options_ErrorCodes>
	{
		return Options::Initialise()
			| Options::Add(Options::App::OptionsProcessor{})
			| Options::Add(Options::Developer::OptionsProcessor{})
			| Options::Add(Options::Equipment::OptionsProcessor{})
			| Options::Add(Options::Mqtt::OptionsProcessor{})
			| Options::Add(Options::Serial::OptionsProcessor{})
			| Options::Add(Options::Web::OptionsProcessor{})
			| Options::Add(Jandy::Options::OptionsProcessor{})
			| Options::Add(Pentair::Options::OptionsProcessor{})
			| Options::Parse(static_cast<int>(args.size()), const_cast<char**>(args.data()))
			| Options::ParseConfigFile()
			| Options::Validate()
			| Options::Process(
				Options::App::OptionsProcessor{},
				Options::Developer::OptionsProcessor{},
				Options::Equipment::OptionsProcessor{},
				Options::Mqtt::OptionsProcessor{},
				Options::Serial::OptionsProcessor{},
				Options::Web::OptionsProcessor{},
				Jandy::Options::OptionsProcessor{},
				Pentair::Options::OptionsProcessor{})
			| Options::Finalise();
	}
}

BOOST_AUTO_TEST_SUITE(TestSuite_OptionsConfigFile)

//=============================================================================
// NO --config: Pipeline works normally (backward compatible)
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_ConfigFile_NoConfigFlag_BackwardCompatible)
{
	auto result = RunPipelineWithConfigFile({ "program" });
	BOOST_REQUIRE(result.has_value());

	// All settings areas should be present with defaults
	const auto& settings = result.value();
	BOOST_CHECK(settings.Has("Application"));
	BOOST_CHECK(settings.Has("Serial"));
	BOOST_CHECK(settings.Has("MQTT"));
	BOOST_CHECK(settings.Has("Web"));
}

//=============================================================================
// VALID CONFIG FILE: All option areas parsed correctly
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_ConfigFile_ValidFile_OptionsLoaded)
{
	TempConfigFile config(
		"serial-port = COM7\n"
		"baudrate = 19200\n"
		"mqtt = true\n"
		"mqtt-host = 192.168.1.100\n"
		"mqtt-port = 1884\n"
		"address = 0.0.0.0\n"
		"disable-https = true\n"
	);

	auto result = RunPipelineWithConfigFile({ "program", "--config", config.path().c_str() });
	BOOST_REQUIRE(result.has_value());

	// Serial
	auto serial = result.value().Get<Options::Serial::SerialSettings>();
	BOOST_REQUIRE(serial.has_value());
	BOOST_CHECK_EQUAL(serial.value().get().serial_port, "COM7");
	BOOST_CHECK_EQUAL(serial.value().get().baud_rate, 19200);

	// MQTT
	auto mqtt = result.value().Get<Options::Mqtt::MqttSettings>();
	BOOST_REQUIRE(mqtt.has_value());
	BOOST_CHECK(mqtt.value().get().enabled);
	BOOST_CHECK_EQUAL(mqtt.value().get().broker_host, "192.168.1.100");
	BOOST_CHECK_EQUAL(mqtt.value().get().broker_port, 1884);

	// Web
	auto web = result.value().Get<Options::Web::WebSettings>();
	BOOST_REQUIRE(web.has_value());
	BOOST_CHECK_EQUAL(web.value().get().bind_address, "0.0.0.0");
	BOOST_CHECK(!web.value().get().https_server_is_enabled);
}

//=============================================================================
// CLI OVERRIDES CONFIG FILE
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_ConfigFile_CLIOverridesConfig)
{
	TempConfigFile config(
		"serial-port = /dev/ttyUSB1\n"
		"baudrate = 19200\n"
	);

	auto result = RunPipelineWithConfigFile({
		"program", "--config", config.path().c_str(), "--serial-port=COM3"
	});
	BOOST_REQUIRE(result.has_value());

	auto serial = result.value().Get<Options::Serial::SerialSettings>();
	BOOST_REQUIRE(serial.has_value());

	// CLI wins over config file
	BOOST_CHECK_EQUAL(serial.value().get().serial_port, "COM3");
	// Config file value used where CLI didn't override
	BOOST_CHECK_EQUAL(serial.value().get().baud_rate, 19200);
}

//=============================================================================
// BOOL SWITCH: true enables flag
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_ConfigFile_BoolSwitchTrue)
{
	TempConfigFile config("mqtt = true\n");

	auto result = RunPipelineWithConfigFile({ "program", "--config", config.path().c_str() });
	BOOST_REQUIRE(result.has_value());

	auto mqtt = result.value().Get<Options::Mqtt::MqttSettings>();
	BOOST_REQUIRE(mqtt.has_value());
	BOOST_CHECK(mqtt.value().get().enabled);
}

//=============================================================================
// BOOL SWITCH: false leaves flag unset
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_ConfigFile_BoolSwitchFalse)
{
	TempConfigFile config("mqtt = false\n");

	auto result = RunPipelineWithConfigFile({ "program", "--config", config.path().c_str() });
	BOOST_REQUIRE(result.has_value());

	auto mqtt = result.value().Get<Options::Mqtt::MqttSettings>();
	BOOST_REQUIRE(mqtt.has_value());
	BOOST_CHECK(!mqtt.value().get().enabled);
}

//=============================================================================
// COMMENTS AND BLANK LINES: Ignored
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_ConfigFile_CommentsAndBlanksIgnored)
{
	TempConfigFile config(
		"# This is a comment\n"
		"\n"
		"  # Indented comment\n"
		"serial-port = COM8\n"
		"\n"
		"# Another comment\n"
		"baudrate = 4800\n"
	);

	auto result = RunPipelineWithConfigFile({ "program", "--config", config.path().c_str() });
	BOOST_REQUIRE(result.has_value());

	auto serial = result.value().Get<Options::Serial::SerialSettings>();
	BOOST_REQUIRE(serial.has_value());
	BOOST_CHECK_EQUAL(serial.value().get().serial_port, "COM8");
	BOOST_CHECK_EQUAL(serial.value().get().baud_rate, 4800);
}

//=============================================================================
// MISSING FILE: Returns OptionsParsingFailed
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_ConfigFile_MissingFile_Fails)
{
	auto result = RunPipelineWithConfigFile({
		"program", "--config", "/nonexistent/path/config.conf"
	});
	BOOST_CHECK(!result.has_value());
}

//=============================================================================
// EMPTY FILE: Succeeds with all defaults
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_ConfigFile_EmptyFile_Succeeds)
{
	TempConfigFile config("");

	auto result = RunPipelineWithConfigFile({ "program", "--config", config.path().c_str() });
	BOOST_REQUIRE(result.has_value());

	// Defaults should still apply
	auto serial = result.value().Get<Options::Serial::SerialSettings>();
	BOOST_REQUIRE(serial.has_value());
	BOOST_CHECK_EQUAL(serial.value().get().baud_rate, 9600);
}

//=============================================================================
// WHITESPACE TOLERANCE: Various spacing around = sign
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_ConfigFile_WhitespaceTolerance)
{
	TempConfigFile config(
		"serial-port=COM9\n"
		"baudrate = 2400\n"
		"  address  =  10.0.0.1  \n"
	);

	auto result = RunPipelineWithConfigFile({ "program", "--config", config.path().c_str() });
	BOOST_REQUIRE(result.has_value());

	auto serial = result.value().Get<Options::Serial::SerialSettings>();
	BOOST_REQUIRE(serial.has_value());
	BOOST_CHECK_EQUAL(serial.value().get().serial_port, "COM9");
	BOOST_CHECK_EQUAL(serial.value().get().baud_rate, 2400);

	auto web = result.value().Get<Options::Web::WebSettings>();
	BOOST_REQUIRE(web.has_value());
	BOOST_CHECK_EQUAL(web.value().get().bind_address, "10.0.0.1");
}

//=============================================================================
// UNKNOWN KEYS: Logged as warning, does not fail
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_ConfigFile_UnknownKeys_DoNotFail)
{
	TempConfigFile config(
		"serial-port = COM10\n"
		"unknown-option = some-value\n"
		"another-unknown = 42\n"
	);

	auto result = RunPipelineWithConfigFile({ "program", "--config", config.path().c_str() });
	BOOST_REQUIRE(result.has_value());

	auto serial = result.value().Get<Options::Serial::SerialSettings>();
	BOOST_REQUIRE(serial.has_value());
	BOOST_CHECK_EQUAL(serial.value().get().serial_port, "COM10");
}

//=============================================================================
// CONFIG + FULL PIPELINE: End-to-end with all processors
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_ConfigFile_FullPipeline_EndToEnd)
{
	TempConfigFile config(
		"serial-port = COM11\n"
		"baudrate = 9600\n"
		"mqtt = true\n"
		"mqtt-host = broker.local\n"
		"mqtt-port = 1885\n"
		"home-assistant = true\n"
		"ha-discovery-prefix = ha\n"
		"address = 192.168.1.1\n"
		"http-port = 8080\n"
		"disable-https = true\n"
	);

	auto result = RunPipelineWithConfigFile({ "program", "--config", config.path().c_str() });
	BOOST_REQUIRE(result.has_value());

	// All settings areas populated
	const auto& settings = result.value();
	BOOST_CHECK(settings.Has("Application"));
	BOOST_CHECK(settings.Has("Developer"));
	BOOST_CHECK(settings.Has("Equipment"));
	BOOST_CHECK(settings.Has("MQTT"));
	BOOST_CHECK(settings.Has("Serial"));
	BOOST_CHECK(settings.Has("Web"));
	BOOST_CHECK(settings.Has("Jandy"));
	BOOST_CHECK(settings.Has("Pentair"));

	// Verify specific settings from config
	auto mqtt = settings.Get<Options::Mqtt::MqttSettings>();
	BOOST_REQUIRE(mqtt.has_value());
	BOOST_CHECK(mqtt.value().get().enabled);
	BOOST_CHECK_EQUAL(mqtt.value().get().broker_host, "broker.local");
	BOOST_CHECK_EQUAL(mqtt.value().get().broker_port, 1885);
	BOOST_CHECK(mqtt.value().get().home_assistant_enabled);
	BOOST_CHECK_EQUAL(mqtt.value().get().ha_discovery_prefix, "ha");

	auto web = settings.Get<Options::Web::WebSettings>();
	BOOST_REQUIRE(web.has_value());
	BOOST_CHECK_EQUAL(web.value().get().bind_address, "192.168.1.1");
	BOOST_CHECK_EQUAL(web.value().get().http_port, 8080);
	BOOST_CHECK(!web.value().get().https_server_is_enabled);
}

//=============================================================================
// API AUTH TOKEN via config file (config-file key == option long name)
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_ConfigFile_ApiAuthToken_Loaded)
{
	TempConfigFile config("api-auth-token = config-file-token\n");

	auto result = RunPipelineWithConfigFile({ "program", "--config", config.path().c_str() });
	BOOST_REQUIRE(result.has_value());

	auto web = result.value().Get<Options::Web::WebSettings>();
	BOOST_REQUIRE(web.has_value());
	BOOST_REQUIRE(web.value().get().ApiAuthToken.has_value());
	BOOST_CHECK_EQUAL(web.value().get().ApiAuthToken.value(), "config-file-token");
}

//=============================================================================
// EMPTY LOG-LEVEL CONFIG VALUE must fail cleanly (regression for the
// severity-validator out-of-bounds read on a bare `loglevel-main =` line).
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_ConfigFile_EmptyLogLevelValue_FailsCleanly)
{
	TempConfigFile config("loglevel-main = \n");

	auto result = RunPipelineWithConfigFile({ "program", "--config", config.path().c_str() });

	// The empty value must be rejected by the validator (pipeline returns an
	// error) rather than crash with an out-of-bounds read.
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END()
