#include <cstdint>
#include <memory>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/keypad_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"

#include "support/unit_test_hublocatorinjector.h"

using namespace AqualinkAutomate::Devices;

namespace
{
	struct KeypadDeviceFixture : public AqualinkAutomate::Test::HubLocatorInjector
	{
		KeypadDeviceFixture()
			: device_type(std::make_shared<JandyDeviceType>(JandyDeviceId(0x08)))
		{
		}

		std::shared_ptr<JandyDeviceType> device_type;
	};
}

BOOST_FIXTURE_TEST_SUITE(KeypadDevice_TestSuite, KeypadDeviceFixture)

// =============================================================================
// Construction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_Emulated)
{
	BOOST_CHECK_NO_THROW(KeypadDevice device(device_type, *this, true));
}

BOOST_AUTO_TEST_CASE(TestConstruction_NonEmulated)
{
	BOOST_CHECK_NO_THROW(KeypadDevice device(device_type, *this, false));
}

// =============================================================================
// Destruction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDestruction_CleanAfterConstruction)
{
	{
		KeypadDevice device(device_type, *this, true);
	}
	BOOST_CHECK(true);
}

// =============================================================================
// Multiple device IDs in the Keypad range
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_DifferentDeviceId)
{
	auto device_type_09 = std::make_shared<JandyDeviceType>(JandyDeviceId(0x09));
	BOOST_CHECK_NO_THROW(KeypadDevice device(device_type_09, *this, true));
}

BOOST_AUTO_TEST_SUITE_END()
