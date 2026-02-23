#include <cstdint>
#include <memory>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/onetouch_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"

#include "support/unit_test_hublocatorinjector.h"

using namespace AqualinkAutomate::Devices;

namespace
{
	struct OneTouchDeviceFixture : public AqualinkAutomate::Test::HubLocatorInjector
	{
		OneTouchDeviceFixture()
			: device_type(std::make_shared<JandyDeviceType>(JandyDeviceId(0x41)))
		{
		}

		std::shared_ptr<JandyDeviceType> device_type;
	};
}

BOOST_FIXTURE_TEST_SUITE(OneTouchDevice_TestSuite, OneTouchDeviceFixture)

// =============================================================================
// Construction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_Emulated)
{
	BOOST_CHECK_NO_THROW(OneTouchDevice device(device_type, *this, true));
}

BOOST_AUTO_TEST_CASE(TestConstruction_NonEmulated)
{
	BOOST_CHECK_NO_THROW(OneTouchDevice device(device_type, *this, false));
}

// =============================================================================
// Destruction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDestruction_CleanAfterConstruction)
{
	{
		OneTouchDevice device(device_type, *this, true);
	}
	BOOST_CHECK(true);
}

// =============================================================================
// Multiple device IDs in the OneTouch range
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_DifferentDeviceId)
{
	auto device_type_40 = std::make_shared<JandyDeviceType>(JandyDeviceId(0x40));
	BOOST_CHECK_NO_THROW(OneTouchDevice device(device_type_40, *this, true));
}

BOOST_AUTO_TEST_SUITE_END()
