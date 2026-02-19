#include <boost/test/unit_test.hpp>

#include <boost/program_options.hpp>

#include "options/options_serial_options.h"
#include "exceptions/exception_options_conflictingoptions.h"

using namespace AqualinkAutomate;
namespace po = boost::program_options;

namespace
{
	/// Helper to create a variables_map from simulated command line arguments using
	/// the Serial OptionsProcessor's options description.
	po::variables_map ParseSerialOptions(Options::Serial::OptionsProcessor& processor, const std::vector<const char*>& args)
	{
		po::options_description desc;
		desc.add(processor.Options());

		po::variables_map vm;
		po::store(po::parse_command_line(static_cast<int>(args.size()), args.data(), desc), vm);
		po::notify(vm);

		return vm;
	}
}

BOOST_AUTO_TEST_SUITE(TestSuite_SerialOptions)

//-----------------------------------------------------------------------------
// DEFAULT VALUES
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_SerialOptions_DefaultSettings)
{
	Options::Serial::SerialSettings settings;

	BOOST_CHECK_EQUAL(settings.serial_port, "/dev/ttyUSB0");
	BOOST_CHECK_EQUAL(settings.baud_rate, 9600);
	BOOST_CHECK(settings.remote_serial_port.empty());
	BOOST_CHECK_EQUAL(settings.use_rfc2217, false);
	BOOST_CHECK_EQUAL(settings.use_rawtcp, false);
}

BOOST_AUTO_TEST_CASE(Test_SerialOptions_ProcessDefaults)
{
	Options::Serial::OptionsProcessor processor;
	auto vm = ParseSerialOptions(processor, { "program" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	const auto& settings = result.value();

	BOOST_CHECK_EQUAL(settings.serial_port, Application::SERIAL_PORT);
	BOOST_CHECK_EQUAL(settings.baud_rate, 9600);
	BOOST_CHECK(settings.remote_serial_port.empty());
}

//-----------------------------------------------------------------------------
// PHYSICAL SERIAL PORT
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_SerialOptions_PhysicalPort)
{
	Options::Serial::OptionsProcessor processor;
	auto vm = ParseSerialOptions(processor, { "program", "--serial-port=COM5" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK_EQUAL(result.value().serial_port, "COM5");
	BOOST_CHECK(result.value().UsingPhysicalSerialPort());
	BOOST_CHECK(!result.value().UsingRemoteSerialPort());
}

BOOST_AUTO_TEST_CASE(Test_SerialOptions_ShortFlag_s)
{
	Options::Serial::OptionsProcessor processor;
	auto vm = ParseSerialOptions(processor, { "program", "-s", "COM3" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK_EQUAL(result.value().serial_port, "COM3");
}

//-----------------------------------------------------------------------------
// REMOTE SERIAL PORT
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_SerialOptions_RemotePort)
{
	Options::Serial::OptionsProcessor processor;
	auto vm = ParseSerialOptions(processor, { "program", "--remote-serial-port=192.168.1.100:2000" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK_EQUAL(result.value().remote_serial_port, "192.168.1.100:2000");
	BOOST_CHECK(result.value().UsingRemoteSerialPort());
	BOOST_CHECK(!result.value().UsingPhysicalSerialPort());
}

BOOST_AUTO_TEST_CASE(Test_SerialOptions_ShortFlag_r)
{
	Options::Serial::OptionsProcessor processor;
	auto vm = ParseSerialOptions(processor, { "program", "-r", "10.0.0.1:3000" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK_EQUAL(result.value().remote_serial_port, "10.0.0.1:3000");
}

//-----------------------------------------------------------------------------
// BAUD RATE
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_SerialOptions_BaudRate)
{
	Options::Serial::OptionsProcessor processor;
	auto vm = ParseSerialOptions(processor, { "program", "--baudrate=19200" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK_EQUAL(result.value().baud_rate, 19200);
}

//-----------------------------------------------------------------------------
// RFC2217 / RAWTCP
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_SerialOptions_Rfc2217Default)
{
	Options::Serial::OptionsProcessor processor;
	auto vm = ParseSerialOptions(processor, { "program" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	// rfc2217 defaults to true
	BOOST_CHECK_EQUAL(result.value().use_rfc2217, true);
	BOOST_CHECK_EQUAL(result.value().use_rawtcp, false);
}

BOOST_AUTO_TEST_CASE(Test_SerialOptions_RawtcpEnabled)
{
	Options::Serial::OptionsProcessor processor;
	auto vm = ParseSerialOptions(processor, { "program", "--rawtcp" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK_EQUAL(result.value().use_rawtcp, true);
}

//-----------------------------------------------------------------------------
// CONFLICT VALIDATION
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_SerialOptions_ConflictPhysicalAndRemote)
{
	Options::Serial::OptionsProcessor processor;
	auto vm = ParseSerialOptions(processor, { "program", "--serial-port=COM5", "--remote-serial-port=192.168.1.1:2000" });

	BOOST_CHECK_THROW(processor.Validate(vm), Exceptions::Options_ConflictingOptions);
}

BOOST_AUTO_TEST_CASE(Test_SerialOptions_ConflictRfc2217AndRawtcp)
{
	Options::Serial::OptionsProcessor processor;
	auto vm = ParseSerialOptions(processor, { "program", "--rfc2177", "--rawtcp" });

	BOOST_CHECK_THROW(processor.Validate(vm), Exceptions::Options_ConflictingOptions);
}

BOOST_AUTO_TEST_CASE(Test_SerialOptions_RemotePortNoConflictWithDefaultedPhysical)
{
	// remote-serial-port with defaulted serial-port should not conflict
	Options::Serial::OptionsProcessor processor;
	auto vm = ParseSerialOptions(processor, { "program", "--remote-serial-port=192.168.1.1:2000" });

	BOOST_CHECK_NO_THROW(processor.Validate(vm));
}

BOOST_AUTO_TEST_SUITE_END()
