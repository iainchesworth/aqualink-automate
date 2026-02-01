#include <boost/test/unit_test.hpp>

#include <boost/program_options.hpp>

#include "options/options_mqtt_options.h"
#include "exceptions/exception_optionparsingfailed.h"
#include "exceptions/exception_options_missingdependency.h"

using namespace AqualinkAutomate;
namespace po = boost::program_options;
/*
BOOST_AUTO_TEST_SUITE(TestSuite_MqttOptions)

BOOST_AUTO_TEST_CASE(Test_MqttOptions_DefaultValues)
{
	BOOST_TEST_MESSAGE("Testing MQTT options default values");
	
	Options::Mqtt::Settings settings;
	
	BOOST_CHECK_EQUAL(settings.broker_host, "localhost");
	BOOST_CHECK_EQUAL(settings.broker_port, 1883);
	BOOST_CHECK_EQUAL(settings.keep_alive_seconds, 60);
	BOOST_CHECK_EQUAL(settings.clean_session, true);
	BOOST_CHECK_EQUAL(settings.qos, 1);
	BOOST_CHECK_EQUAL(settings.retain, false);
	BOOST_CHECK_EQUAL(settings.is_enabled, false);
	BOOST_CHECK_EQUAL(settings.use_tls, false);
	BOOST_CHECK_EQUAL(settings.auto_reconnect, true);
	BOOST_CHECK_EQUAL(settings.reconnect_delay_seconds, 5);
	BOOST_CHECK_EQUAL(settings.max_reconnect_attempts, 0);
	BOOST_CHECK_EQUAL(settings.verify_hostname, true);
}

BOOST_AUTO_TEST_CASE(Test_MqttOptions_BasicConfiguration)
{
	BOOST_TEST_MESSAGE("Testing basic MQTT options configuration");
	
	auto options_desc = Options::Mqtt::Options();
	
	po::variables_map vm;
	
	// Simulate command line arguments
	const char* argv[] = {
		"program",
		"--mqtt.enable",
		"--mqtt.broker-host=broker.example.com",
		"--mqtt.broker-port=8883",
		"--mqtt.client-id=my-client"
	};
	int argc = sizeof(argv) / sizeof(argv[0]);
	
	po::store(po::parse_command_line(argc, argv, options_desc), vm);
	po::notify(vm);
	
	BOOST_CHECK_NO_THROW(Options::Mqtt::ValidateOptions(vm));
	
	auto settings = Options::Mqtt::HandleOptions(vm);
	
	BOOST_CHECK_EQUAL(settings.is_enabled, true);
	BOOST_CHECK_EQUAL(settings.broker_host, "broker.example.com");
	BOOST_CHECK_EQUAL(settings.broker_port, 8883);
	BOOST_CHECK_EQUAL(settings.client_id, "my-client");
}

BOOST_AUTO_TEST_CASE(Test_MqttOptions_AuthenticationConfiguration)
{
	BOOST_TEST_MESSAGE("Testing MQTT authentication configuration");
	
	auto options_desc = Options::Mqtt::Options();
	
	po::variables_map vm;
	
	const char* argv[] = {
		"program",
		"--mqtt.enable",
		"--mqtt.username=testuser",
		"--mqtt.password=testpass"
	};
	int argc = sizeof(argv) / sizeof(argv[0]);
	
	po::store(po::parse_command_line(argc, argv, options_desc), vm);
	po::notify(vm);
	
	BOOST_CHECK_NO_THROW(Options::Mqtt::ValidateOptions(vm));
	
	auto settings = Options::Mqtt::HandleOptions(vm);
	
	BOOST_CHECK(settings.username.has_value());
	BOOST_CHECK(settings.password.has_value());
	BOOST_CHECK_EQUAL(*settings.username, "testuser");
	BOOST_CHECK_EQUAL(*settings.password, "testpass");
}

BOOST_AUTO_TEST_CASE(Test_MqttOptions_TlsConfiguration)
{
	BOOST_TEST_MESSAGE("Testing MQTT TLS configuration");
	
	auto options_desc = Options::Mqtt::Options();
	
	po::variables_map vm;
	
	const char* argv[] = {
		"program",
		"--mqtt.enable",
		"--mqtt.use-tls",
		"--mqtt.ca-file=/path/to/ca.pem",
		"--mqtt.cert-file=/path/to/cert.pem",
		"--mqtt.key-file=/path/to/key.pem",
		"--mqtt.verify-hostname=false"
	};
	int argc = sizeof(argv) / sizeof(argv[0]);
	
	po::store(po::parse_command_line(argc, argv, options_desc), vm);
	po::notify(vm);
	
	BOOST_CHECK_NO_THROW(Options::Mqtt::ValidateOptions(vm));
	
	auto settings = Options::Mqtt::HandleOptions(vm);
	
	BOOST_CHECK_EQUAL(settings.use_tls, true);
	BOOST_CHECK(settings.ca_file.has_value());
	BOOST_CHECK(settings.cert_file.has_value());
	BOOST_CHECK(settings.key_file.has_value());
	BOOST_CHECK_EQUAL(*settings.ca_file, "/path/to/ca.pem");
	BOOST_CHECK_EQUAL(*settings.cert_file, "/path/to/cert.pem");
	BOOST_CHECK_EQUAL(*settings.key_file, "/path/to/key.pem");
	BOOST_CHECK_EQUAL(settings.verify_hostname, false);
}

BOOST_AUTO_TEST_CASE(Test_MqttOptions_QualityOfServiceConfiguration)
{
	BOOST_TEST_MESSAGE("Testing MQTT QoS configuration");
	
	auto options_desc = Options::Mqtt::Options();
	
	po::variables_map vm;
	
	const char* argv[] = {
		"program",
		"--mqtt.enable",
		"--mqtt.qos=2",
		"--mqtt.retain",
		"--mqtt.clean-session=false"
	};
	int argc = sizeof(argv) / sizeof(argv[0]);
	
	po::store(po::parse_command_line(argc, argv, options_desc), vm);
	po::notify(vm);
	
	BOOST_CHECK_NO_THROW(Options::Mqtt::ValidateOptions(vm));
	
	auto settings = Options::Mqtt::HandleOptions(vm);
	
	BOOST_CHECK_EQUAL(settings.qos, 2);
	BOOST_CHECK_EQUAL(settings.retain, true);
	BOOST_CHECK_EQUAL(settings.clean_session, false);
}

BOOST_AUTO_TEST_CASE(Test_MqttOptions_TopicConfiguration)
{
	BOOST_TEST_MESSAGE("Testing MQTT topic configuration");
	
	auto options_desc = Options::Mqtt::Options();
	
	po::variables_map vm;
	
	const char* argv[] = {
		"program",
		"--mqtt.enable",
		"--mqtt.topic-prefix=mypool"
	};
	int argc = sizeof(argv) / sizeof(argv[0]);
	
	po::store(po::parse_command_line(argc, argv, options_desc), vm);
	po::notify(vm);
	
	BOOST_CHECK_NO_THROW(Options::Mqtt::ValidateOptions(vm));
	
	auto settings = Options::Mqtt::HandleOptions(vm);
	
	BOOST_CHECK_EQUAL(settings.topic_prefix, "mypool");
}

BOOST_AUTO_TEST_CASE(Test_MqttOptions_ReconnectionConfiguration)
{
	BOOST_TEST_MESSAGE("Testing MQTT reconnection configuration");
	
	auto options_desc = Options::Mqtt::Options();
	
	po::variables_map vm;
	
	const char* argv[] = {
		"program",
		"--mqtt.enable",
		"--mqtt.auto-reconnect=false",
		"--mqtt.reconnect-delay=10",
		"--mqtt.max-reconnect-attempts=5"
	};
	int argc = sizeof(argv) / sizeof(argv[0]);
	
	po::store(po::parse_command_line(argc, argv, options_desc), vm);
	po::notify(vm);
	
	BOOST_CHECK_NO_THROW(Options::Mqtt::ValidateOptions(vm));
	
	auto settings = Options::Mqtt::HandleOptions(vm);
	
	BOOST_CHECK_EQUAL(settings.auto_reconnect, false);
	BOOST_CHECK_EQUAL(settings.reconnect_delay_seconds, 10);
	BOOST_CHECK_EQUAL(settings.max_reconnect_attempts, 5);
}

BOOST_AUTO_TEST_CASE(Test_MqttOptions_InvalidQoSLevel)
{
	BOOST_TEST_MESSAGE("Testing MQTT options validation with invalid QoS level");
	
	auto options_desc = Options::Mqtt::Options();
	
	po::variables_map vm;
	
	const char* argv[] = {
		"program",
		"--mqtt.enable",
		"--mqtt.qos=3"  // Invalid QoS level
	};
	int argc = sizeof(argv) / sizeof(argv[0]);
	
	po::store(po::parse_command_line(argc, argv, options_desc), vm);
	po::notify(vm);
	
	// Should throw validation error
	BOOST_CHECK_THROW(Options::Mqtt::ValidateOptions(vm), Exceptions::OptionParsingFailed);
}

BOOST_AUTO_TEST_CASE(Test_MqttOptions_TlsCertificateWithoutKey)
{
	BOOST_TEST_MESSAGE("Testing MQTT options validation with certificate but no key");
	
	auto options_desc = Options::Mqtt::Options();
	
	po::variables_map vm;
	
	const char* argv[] = {
		"program",
		"--mqtt.enable",
		"--mqtt.use-tls",
		"--mqtt.cert-file=/path/to/cert.pem"
		// Missing --mqtt.key-file
	};
	int argc = sizeof(argv) / sizeof(argv[0]);
	
	po::store(po::parse_command_line(argc, argv, options_desc), vm);
	po::notify(vm);
	
	// Should throw validation error
	BOOST_CHECK_THROW(Options::Mqtt::ValidateOptions(vm), Exceptions::Options_MissingDependency);
}

BOOST_AUTO_TEST_CASE(Test_MqttOptions_TlsKeyWithoutCertificate)
{
	BOOST_TEST_MESSAGE("Testing MQTT options validation with key but no certificate");
	
	auto options_desc = Options::Mqtt::Options();
	
	po::variables_map vm;
	
	const char* argv[] = {
		"program",
		"--mqtt.enable",
		"--mqtt.use-tls",
		"--mqtt.key-file=/path/to/key.pem"
		// Missing --mqtt.cert-file
	};
	int argc = sizeof(argv) / sizeof(argv[0]);
	
	po::store(po::parse_command_line(argc, argv, options_desc), vm);
	po::notify(vm);
	
	// Should throw validation error
	BOOST_CHECK_THROW(Options::Mqtt::ValidateOptions(vm), Exceptions::Options_MissingDependency);
}

BOOST_AUTO_TEST_CASE(Test_MqttOptions_PasswordWithoutUsername)
{
	BOOST_TEST_MESSAGE("Testing MQTT options validation with password but no username");
	
	auto options_desc = Options::Mqtt::Options();
	
	po::variables_map vm;
	
	const char* argv[] = {
		"program",
		"--mqtt.enable",
		"--mqtt.password=secret"
		// Missing --mqtt.username
	};
	int argc = sizeof(argv) / sizeof(argv[0]);
	
	po::store(po::parse_command_line(argc, argv, options_desc), vm);
	po::notify(vm);
	
	// Should throw validation error
	BOOST_CHECK_THROW(Options::Mqtt::ValidateOptions(vm), Exceptions::Options_MissingDependency);
}

BOOST_AUTO_TEST_CASE(Test_MqttOptions_UsernameWithoutPassword)
{
	BOOST_TEST_MESSAGE("Testing MQTT options validation with username but no password");
	
	auto options_desc = Options::Mqtt::Options();
	
	po::variables_map vm;
	
	const char* argv[] = {
		"program",
		"--mqtt.enable",
		"--mqtt.username=testuser"
		// Missing --mqtt.password is allowed
	};
	int argc = sizeof(argv) / sizeof(argv[0]);
	
	po::store(po::parse_command_line(argc, argv, options_desc), vm);
	po::notify(vm);
	
	// Should not throw - username without password is valid
	BOOST_CHECK_NO_THROW(Options::Mqtt::ValidateOptions(vm));
	
	auto settings = Options::Mqtt::HandleOptions(vm);
	BOOST_CHECK(settings.username.has_value());
	BOOST_CHECK(!settings.password.has_value());
}

BOOST_AUTO_TEST_CASE(Test_MqttOptions_ValidQoSLevels)
{
	BOOST_TEST_MESSAGE("Testing MQTT options validation with valid QoS levels");
	
	auto options_desc = Options::Mqtt::Options();
	
	// Test QoS 0
	{
		po::variables_map vm;
		const char* argv[] = {"program", "--mqtt.enable", "--mqtt.qos=0"};
		int argc = sizeof(argv) / sizeof(argv[0]);
		
		po::store(po::parse_command_line(argc, argv, options_desc), vm);
		po::notify(vm);
		
		BOOST_CHECK_NO_THROW(Options::Mqtt::ValidateOptions(vm));
		auto settings = Options::Mqtt::HandleOptions(vm);
		BOOST_CHECK_EQUAL(settings.qos, 0);
	}
	
	// Test QoS 1
	{
		po::variables_map vm;
		const char* argv[] = {"program", "--mqtt.enable", "--mqtt.qos=1"};
		int argc = sizeof(argv) / sizeof(argv[0]);
		
		po::store(po::parse_command_line(argc, argv, options_desc), vm);
		po::notify(vm);
		
		BOOST_CHECK_NO_THROW(Options::Mqtt::ValidateOptions(vm));
		auto settings = Options::Mqtt::HandleOptions(vm);
		BOOST_CHECK_EQUAL(settings.qos, 1);
	}
	
	// Test QoS 2
	{
		po::variables_map vm;
		const char* argv[] = {"program", "--mqtt.enable", "--mqtt.qos=2"};
		int argc = sizeof(argv) / sizeof(argv[0]);
		
		po::store(po::parse_command_line(argc, argv, options_desc), vm);
		po::notify(vm);
		
		BOOST_CHECK_NO_THROW(Options::Mqtt::ValidateOptions(vm));
		auto settings = Options::Mqtt::HandleOptions(vm);
		BOOST_CHECK_EQUAL(settings.qos, 2);
	}
}

BOOST_AUTO_TEST_CASE(Test_MqttOptions_CompleteConfiguration)
{
	BOOST_TEST_MESSAGE("Testing complete MQTT options configuration");
	
	auto options_desc = Options::Mqtt::Options();
	
	po::variables_map vm;
	
	const char* argv[] = {
		"program",
		"--mqtt.enable",
		"--mqtt.broker-host=secure.broker.com",
		"--mqtt.broker-port=8883",
		"--mqtt.client-id=pool-controller-001",
		"--mqtt.username=pool_user",
		"--mqtt.password=secure_password",
		"--mqtt.keep-alive=120",
		"--mqtt.clean-session=false",
		"--mqtt.qos=1",
		"--mqtt.retain",
		"--mqtt.topic-prefix=home/pool",
		"--mqtt.use-tls",
		"--mqtt.ca-file=/etc/ssl/certs/ca.pem",
		"--mqtt.cert-file=/etc/ssl/certs/client.pem",
		"--mqtt.key-file=/etc/ssl/private/client.key",
		"--mqtt.verify-hostname=true",
		"--mqtt.auto-reconnect=true",
		"--mqtt.reconnect-delay=30",
		"--mqtt.max-reconnect-attempts=10"
	};
	int argc = sizeof(argv) / sizeof(argv[0]);
	
	po::store(po::parse_command_line(argc, argv, options_desc), vm);
	po::notify(vm);
	
	BOOST_CHECK_NO_THROW(Options::Mqtt::ValidateOptions(vm));
	
	auto settings = Options::Mqtt::HandleOptions(vm);
	
	// Verify all settings
	BOOST_CHECK_EQUAL(settings.is_enabled, true);
	BOOST_CHECK_EQUAL(settings.broker_host, "secure.broker.com");
	BOOST_CHECK_EQUAL(settings.broker_port, 8883);
	BOOST_CHECK_EQUAL(settings.client_id, "pool-controller-001");
	BOOST_CHECK_EQUAL(*settings.username, "pool_user");
	BOOST_CHECK_EQUAL(*settings.password, "secure_password");
	BOOST_CHECK_EQUAL(settings.keep_alive_seconds, 120);
	BOOST_CHECK_EQUAL(settings.clean_session, false);
	BOOST_CHECK_EQUAL(settings.qos, 1);
	BOOST_CHECK_EQUAL(settings.retain, true);
	BOOST_CHECK_EQUAL(settings.topic_prefix, "home/pool");
	BOOST_CHECK_EQUAL(settings.use_tls, true);
	BOOST_CHECK_EQUAL(*settings.ca_file, "/etc/ssl/certs/ca.pem");
	BOOST_CHECK_EQUAL(*settings.cert_file, "/etc/ssl/certs/client.pem");
	BOOST_CHECK_EQUAL(*settings.key_file, "/etc/ssl/private/client.key");
	BOOST_CHECK_EQUAL(settings.verify_hostname, true);
	BOOST_CHECK_EQUAL(settings.auto_reconnect, true);
	BOOST_CHECK_EQUAL(settings.reconnect_delay_seconds, 30);
	BOOST_CHECK_EQUAL(settings.max_reconnect_attempts, 10);
}

BOOST_AUTO_TEST_SUITE_END()*/