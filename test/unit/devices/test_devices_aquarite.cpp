#include <chrono>
#include <cstdint>
#include <memory>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/aquarite_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"

#include "support/unit_test_hublocatorinjector.h"
#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Devices;

namespace
{
	struct AquariteDeviceFixture : public AqualinkAutomate::Test::HubLocatorInjector
	{
		AquariteDeviceFixture()
			: device_type(std::make_shared<JandyDeviceType>(JandyDeviceId(0x50)))
		{
		}

		std::shared_ptr<JandyDeviceType> device_type;
	};
}

BOOST_FIXTURE_TEST_SUITE(AquariteDevice_TestSuite, AquariteDeviceFixture)

// =============================================================================
// Construction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_DefaultValues)
{
	AquariteDevice device(device_type, *this);

	auto requested = device.RequestedGeneratingLevel();
	auto reported = device.ReportedGeneratingLevel();
	auto salt = device.ReportedSaltConcentration();

	BOOST_CHECK_EQUAL(requested.value, static_cast<uint8_t>(0));
	BOOST_CHECK_EQUAL(reported.value, static_cast<uint8_t>(0));
	BOOST_CHECK_EQUAL(salt.value, static_cast<uint16_t>(0));
}

BOOST_AUTO_TEST_CASE(TestConstruction_WithInitialValues)
{
	AquariteDevice device(device_type, *this, 50, 45, 3200);

	auto requested = device.RequestedGeneratingLevel();
	auto reported = device.ReportedGeneratingLevel();
	auto salt = device.ReportedSaltConcentration();

	// Note: requested goes through debouncer, so may differ from initial
	BOOST_CHECK_EQUAL(reported.value, static_cast<uint8_t>(45));
	BOOST_CHECK_EQUAL(salt.value, static_cast<uint16_t>(3200));
}

// =============================================================================
// Public API: Setters and Getters
// =============================================================================

BOOST_AUTO_TEST_CASE(TestRequestedGeneratingLevel_SetAndGet_Debounced)
{
	AquariteDevice device(device_type, *this);

	// RequestedGeneratingLevel is debounced (threshold=10), so a single set may not
	// immediately update. Set it repeatedly to exceed the debounce threshold.
	for (uint32_t i = 0; i < 15; ++i)
	{
		device.RequestedGeneratingLevel(75);
	}

	auto result = device.RequestedGeneratingLevel();
	BOOST_CHECK_EQUAL(result.value, static_cast<uint8_t>(75));
}

BOOST_AUTO_TEST_CASE(TestReportedGeneratingLevel_SetAndGet)
{
	AquariteDevice device(device_type, *this);

	device.ReportedGeneratingLevel(60);

	auto result = device.ReportedGeneratingLevel();
	BOOST_CHECK_EQUAL(result.value, static_cast<uint8_t>(60));
}

BOOST_AUTO_TEST_CASE(TestReportedSaltConcentration_SetAndGet)
{
	AquariteDevice device(device_type, *this);

	device.ReportedSaltConcentration(3500);

	auto result = device.ReportedSaltConcentration();
	BOOST_CHECK_EQUAL(result.value, static_cast<uint16_t>(3500));
}

BOOST_AUTO_TEST_CASE(TestReportedGeneratingLevel_TimestampIsRecent)
{
	auto before = std::chrono::system_clock::now();

	AquariteDevice device(device_type, *this);
	device.ReportedGeneratingLevel(50);

	auto after = std::chrono::system_clock::now();

	auto result = device.ReportedGeneratingLevel();
	BOOST_CHECK(result.timestamp >= before);
	BOOST_CHECK(result.timestamp <= after);
}

BOOST_AUTO_TEST_CASE(TestReportedSaltConcentration_TimestampIsRecent)
{
	auto before = std::chrono::system_clock::now();

	AquariteDevice device(device_type, *this);
	device.ReportedSaltConcentration(2800);

	auto after = std::chrono::system_clock::now();

	auto result = device.ReportedSaltConcentration();
	BOOST_CHECK(result.timestamp >= before);
	BOOST_CHECK(result.timestamp <= after);
}

// =============================================================================
// Boundary Values
// =============================================================================

BOOST_AUTO_TEST_CASE(TestReportedGeneratingLevel_ZeroPercent)
{
	AquariteDevice device(device_type, *this);
	device.ReportedGeneratingLevel(0);
	BOOST_CHECK_EQUAL(device.ReportedGeneratingLevel().value, static_cast<uint8_t>(0));
}

BOOST_AUTO_TEST_CASE(TestReportedGeneratingLevel_HundredPercent)
{
	AquariteDevice device(device_type, *this);
	device.ReportedGeneratingLevel(100);
	BOOST_CHECK_EQUAL(device.ReportedGeneratingLevel().value, static_cast<uint8_t>(100));
}

BOOST_AUTO_TEST_CASE(TestReportedSaltConcentration_Zero)
{
	AquariteDevice device(device_type, *this);
	device.ReportedSaltConcentration(0);
	BOOST_CHECK_EQUAL(device.ReportedSaltConcentration().value, static_cast<uint16_t>(0));
}

BOOST_AUTO_TEST_CASE(TestReportedSaltConcentration_MaxPPM)
{
	AquariteDevice device(device_type, *this);
	device.ReportedSaltConcentration(65535);
	BOOST_CHECK_EQUAL(device.ReportedSaltConcentration().value, static_cast<uint16_t>(65535));
}

// =============================================================================
// Multiple Updates
// =============================================================================

BOOST_AUTO_TEST_CASE(TestMultipleUpdates_ReportedGeneratingLevel)
{
	AquariteDevice device(device_type, *this);

	device.ReportedGeneratingLevel(10);
	BOOST_CHECK_EQUAL(device.ReportedGeneratingLevel().value, static_cast<uint8_t>(10));

	device.ReportedGeneratingLevel(50);
	BOOST_CHECK_EQUAL(device.ReportedGeneratingLevel().value, static_cast<uint8_t>(50));

	device.ReportedGeneratingLevel(0);
	BOOST_CHECK_EQUAL(device.ReportedGeneratingLevel().value, static_cast<uint8_t>(0));
}

BOOST_AUTO_TEST_CASE(TestMultipleUpdates_SaltConcentration)
{
	AquariteDevice device(device_type, *this);

	device.ReportedSaltConcentration(2800);
	device.ReportedSaltConcentration(3200);
	device.ReportedSaltConcentration(3500);

	BOOST_CHECK_EQUAL(device.ReportedSaltConcentration().value, static_cast<uint16_t>(3500));
}

BOOST_AUTO_TEST_SUITE_END()
