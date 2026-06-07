#include <boost/test/unit_test.hpp>

#include <boost/program_options.hpp>

#include "options/options.h"
#include "jandy/options/options_jandy.h"
#include "pentair/options/options_pentair.h"

using namespace AqualinkAutomate;
namespace po = boost::program_options;

namespace
{
	auto RunFullPipeline(const std::vector<const char*>& args)
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
			| Options::Notify()
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

BOOST_AUTO_TEST_SUITE(TestSuite_OptionsJandy)

//=============================================================================
// CSV DEVICE TYPES: comma-separated parsing
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_Jandy_SingleDeviceType)
{
	auto result = RunFullPipeline({ "program", "--jandy-device-type=IAQ" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());

	const auto& devices = jandy.value().get().emulated_devices;
	BOOST_REQUIRE_EQUAL(devices.size(), 1);
	BOOST_CHECK(devices[0].first == Devices::JandyEmulatedDeviceTypes::IAQ);
}

BOOST_AUTO_TEST_CASE(Test_Jandy_TwoDeviceTypes)
{
	auto result = RunFullPipeline({ "program", "--jandy-device-type=iaq,onetouch" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());

	const auto& devices = jandy.value().get().emulated_devices;
	BOOST_REQUIRE_EQUAL(devices.size(), 2);
	BOOST_CHECK(devices[0].first == Devices::JandyEmulatedDeviceTypes::IAQ);
	BOOST_CHECK(devices[1].first == Devices::JandyEmulatedDeviceTypes::OneTouch);
}

BOOST_AUTO_TEST_CASE(Test_Jandy_DeviceTypes_CaseInsensitive)
{
	auto result = RunFullPipeline({ "program", "--jandy-device-type=iaq" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());

	const auto& devices = jandy.value().get().emulated_devices;
	BOOST_REQUIRE_EQUAL(devices.size(), 1);
	BOOST_CHECK(devices[0].first == Devices::JandyEmulatedDeviceTypes::IAQ);
}

BOOST_AUTO_TEST_CASE(Test_Jandy_DeviceTypes_WhitespaceAroundComma)
{
	auto result = RunFullPipeline({ "program", "--jandy-device-type=iaq, onetouch" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());

	const auto& devices = jandy.value().get().emulated_devices;
	BOOST_REQUIRE_EQUAL(devices.size(), 2);
	BOOST_CHECK(devices[0].first == Devices::JandyEmulatedDeviceTypes::IAQ);
	BOOST_CHECK(devices[1].first == Devices::JandyEmulatedDeviceTypes::OneTouch);
}

BOOST_AUTO_TEST_CASE(Test_Jandy_DeviceTypes_InvalidType_Throws)
{
	BOOST_CHECK_THROW((void)RunFullPipeline({ "program", "--jandy-device-type=bogus" }), boost::program_options::validation_error); // NOLINT(bugprone-unused-return-value)
}

//=============================================================================
// CSV DEVICE IDS: comma-separated parsing
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_Jandy_SingleDeviceId)
{
	auto result = RunFullPipeline({ "program", "--jandy-device-type=IAQ", "--jandy-device-id=0xA1" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());

	const auto& devices = jandy.value().get().emulated_devices;
	BOOST_REQUIRE_EQUAL(devices.size(), 1);
	BOOST_CHECK_EQUAL(devices[0].second.Id()(), 0xA1);
}

BOOST_AUTO_TEST_CASE(Test_Jandy_TwoDeviceIds)
{
	auto result = RunFullPipeline({ "program", "--jandy-device-type=iaq,onetouch", "--jandy-device-id=0xa1,0x41" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());

	const auto& devices = jandy.value().get().emulated_devices;
	BOOST_REQUIRE_EQUAL(devices.size(), 2);
	BOOST_CHECK_EQUAL(devices[0].second.Id()(), 0xA1);
	BOOST_CHECK_EQUAL(devices[1].second.Id()(), 0x41);
}

BOOST_AUTO_TEST_CASE(Test_Jandy_DeviceIds_InvalidHex_Throws)
{
	BOOST_CHECK_THROW((void)RunFullPipeline({ "program", "--jandy-device-type=IAQ", "--jandy-device-id=0xZZ" }), boost::program_options::validation_error); // NOLINT(bugprone-unused-return-value)
}

BOOST_AUTO_TEST_CASE(Test_Jandy_DeviceIds_MissingPrefix_Throws)
{
	BOOST_CHECK_THROW((void)RunFullPipeline({ "program", "--jandy-device-type=IAQ", "--jandy-device-id=A1" }), boost::program_options::validation_error); // NOLINT(bugprone-unused-return-value)
}

//=============================================================================
// DEFAULT IDS: types without explicit IDs get defaults
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_Jandy_DefaultIds_Assigned)
{
	auto result = RunFullPipeline({ "program", "--jandy-device-type=iaq,onetouch" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());

	const auto& devices = jandy.value().get().emulated_devices;
	BOOST_REQUIRE_EQUAL(devices.size(), 2);
	BOOST_CHECK_EQUAL(devices[0].second.Id()(), 0xA1);  // Default IAQ id
	BOOST_CHECK_EQUAL(devices[1].second.Id()(), 0x41);  // Default OneTouch id
}

BOOST_AUTO_TEST_SUITE_END()
