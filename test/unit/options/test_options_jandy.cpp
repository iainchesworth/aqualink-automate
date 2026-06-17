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
// DEVICE TYPES: space-separated (multitoken) parsing via the boost::po validator
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_Jandy_SingleDeviceType)
{
	auto result = RunFullPipeline({ "program", "--jandy-device-type", "IAQ" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());

	const auto& devices = jandy.value().get().emulated_devices;
	BOOST_REQUIRE_EQUAL(devices.size(), 1);
	BOOST_CHECK(devices[0].first == Devices::JandyEmulatedDeviceTypes::IAQ);
}

BOOST_AUTO_TEST_CASE(Test_Jandy_TwoDeviceTypes)
{
	auto result = RunFullPipeline({ "program", "--jandy-device-type", "iaq", "onetouch" });
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
	auto result = RunFullPipeline({ "program", "--jandy-device-type", "iaq" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());

	const auto& devices = jandy.value().get().emulated_devices;
	BOOST_REQUIRE_EQUAL(devices.size(), 1);
	BOOST_CHECK(devices[0].first == Devices::JandyEmulatedDeviceTypes::IAQ);
}

BOOST_AUTO_TEST_CASE(Test_Jandy_ThreeDeviceTypes)
{
	auto result = RunFullPipeline({ "program", "--jandy-device-type", "iaq", "onetouch", "serialadapter" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());

	const auto& devices = jandy.value().get().emulated_devices;
	BOOST_REQUIRE_EQUAL(devices.size(), 3);
	BOOST_CHECK(devices[0].first == Devices::JandyEmulatedDeviceTypes::IAQ);
	BOOST_CHECK(devices[1].first == Devices::JandyEmulatedDeviceTypes::OneTouch);
	BOOST_CHECK(devices[2].first == Devices::JandyEmulatedDeviceTypes::SerialAdapter);
}

BOOST_AUTO_TEST_CASE(Test_Jandy_DeviceTypes_InvalidType_ReturnsError)
{
	// The validator throws validation_error during parsing; the pipeline converts
	// that into a clean OptionsParsingFailed result (rather than an uncaught throw).
	auto result = RunFullPipeline({ "program", "--jandy-device-type", "bogus" });
	BOOST_CHECK(!result.has_value());
}

//=============================================================================
// PRESENCE GATING opt-out (--jandy-disable-presence-gating)
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_Jandy_PresenceGating_DefaultsEnabled)
{
	auto result = RunFullPipeline({ "program" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());
	BOOST_CHECK(!jandy.value().get().disable_presence_gating);
}

BOOST_AUTO_TEST_CASE(Test_Jandy_PresenceGating_DisableFlag)
{
	auto result = RunFullPipeline({ "program", "--jandy-disable-presence-gating" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());
	BOOST_CHECK(jandy.value().get().disable_presence_gating);
}

//=============================================================================
// DEVICE IDS: space-separated (multitoken) parsing via the boost::po validator
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_Jandy_SingleDeviceId)
{
	auto result = RunFullPipeline({ "program", "--jandy-device-type", "IAQ", "--jandy-device-id", "0xA1" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());

	const auto& devices = jandy.value().get().emulated_devices;
	BOOST_REQUIRE_EQUAL(devices.size(), 1);
	BOOST_CHECK_EQUAL(devices[0].second.Id()(), 0xA1);
}

BOOST_AUTO_TEST_CASE(Test_Jandy_TwoDeviceIds)
{
	auto result = RunFullPipeline({ "program", "--jandy-device-type", "iaq", "onetouch", "--jandy-device-id", "0xa1", "0x41" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());

	const auto& devices = jandy.value().get().emulated_devices;
	BOOST_REQUIRE_EQUAL(devices.size(), 2);
	BOOST_CHECK_EQUAL(devices[0].second.Id()(), 0xA1);
	BOOST_CHECK_EQUAL(devices[1].second.Id()(), 0x41);
}

BOOST_AUTO_TEST_CASE(Test_Jandy_DeviceIds_InvalidHex_ReturnsError)
{
	auto result = RunFullPipeline({ "program", "--jandy-device-type", "IAQ", "--jandy-device-id", "0xZZ" });
	BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(Test_Jandy_DeviceIds_MissingPrefix_ReturnsError)
{
	auto result = RunFullPipeline({ "program", "--jandy-device-type", "IAQ", "--jandy-device-id", "A1" });
	BOOST_CHECK(!result.has_value());
}

//=============================================================================
// DEFAULT IDS: types without explicit IDs get defaults
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_Jandy_DefaultIds_Assigned)
{
	auto result = RunFullPipeline({ "program", "--jandy-device-type", "iaq", "onetouch" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());

	const auto& devices = jandy.value().get().emulated_devices;
	BOOST_REQUIRE_EQUAL(devices.size(), 2);
	BOOST_CHECK_EQUAL(devices[0].second.Id()(), 0xA1);  // Default IAQ id
	BOOST_CHECK_EQUAL(devices[1].second.Id()(), 0x41);  // Default OneTouch id
}

//=============================================================================
// CHLORINATOR SETPOINT REFRESH INTERVAL (--chlorinator-setpoint-refresh-interval)
//=============================================================================

BOOST_AUTO_TEST_CASE(Test_Jandy_ChlorinatorSetpointRefresh_DefaultsTo300)
{
	auto result = RunFullPipeline({ "program" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());
	BOOST_CHECK_EQUAL(jandy.value().get().chlorinator_setpoint_refresh_interval, 300u);
}

BOOST_AUTO_TEST_CASE(Test_Jandy_ChlorinatorSetpointRefresh_ExplicitValue)
{
	auto result = RunFullPipeline({ "program", "--chlorinator-setpoint-refresh-interval", "120" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());
	BOOST_CHECK_EQUAL(jandy.value().get().chlorinator_setpoint_refresh_interval, 120u);
}

BOOST_AUTO_TEST_CASE(Test_Jandy_ChlorinatorSetpointRefresh_ZeroDisables)
{
	auto result = RunFullPipeline({ "program", "--chlorinator-setpoint-refresh-interval", "0" });
	BOOST_REQUIRE(result.has_value());

	auto jandy = result.value().Get<Jandy::Options::JandySettings>();
	BOOST_REQUIRE(jandy.has_value());
	BOOST_CHECK_EQUAL(jandy.value().get().chlorinator_setpoint_refresh_interval, 0u);
}

BOOST_AUTO_TEST_SUITE_END()
