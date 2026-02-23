#include <cstdint>
#include <memory>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/pda_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"

#include "support/unit_test_hublocatorinjector.h"

using namespace AqualinkAutomate::Devices;

namespace
{
	struct PDADeviceFixture : public AqualinkAutomate::Test::HubLocatorInjector
	{
		PDADeviceFixture()
			: device_type(std::make_shared<JandyDeviceType>(JandyDeviceId(0x60)))
		{
		}

		std::shared_ptr<JandyDeviceType> device_type;
	};
}

BOOST_FIXTURE_TEST_SUITE(PDADevice_TestSuite, PDADeviceFixture)

// =============================================================================
// Construction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_Emulated)
{
	BOOST_CHECK_NO_THROW(PDADevice device(device_type, *this, true));
}

BOOST_AUTO_TEST_CASE(TestConstruction_NonEmulated)
{
	BOOST_CHECK_NO_THROW(PDADevice device(device_type, *this, false));
}

// =============================================================================
// Destruction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDestruction_CleanAfterConstruction)
{
	{
		PDADevice device(device_type, *this, true);
	}
	BOOST_CHECK(true);
}

// =============================================================================
// Multiple device IDs in the PDA range
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_DifferentDeviceId)
{
	auto device_type_61 = std::make_shared<JandyDeviceType>(JandyDeviceId(0x61));
	BOOST_CHECK_NO_THROW(PDADevice device(device_type_61, *this, true));
}

BOOST_AUTO_TEST_SUITE_END()
