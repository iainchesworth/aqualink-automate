#include <cstdint>
#include <memory>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/iaq_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"

#include "support/unit_test_hublocatorinjector.h"

using namespace AqualinkAutomate::Devices;

namespace
{
	struct IAQDeviceFixture : public AqualinkAutomate::Test::HubLocatorInjector
	{
		IAQDeviceFixture()
			: device_type(std::make_shared<JandyDeviceType>(JandyDeviceId(0x33)))
		{
		}

		std::shared_ptr<JandyDeviceType> device_type;
	};
}

BOOST_FIXTURE_TEST_SUITE(IAQDevice_TestSuite, IAQDeviceFixture)

// =============================================================================
// Construction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_Emulated)
{
	BOOST_CHECK_NO_THROW(IAQDevice device(device_type, *this, true));
}

BOOST_AUTO_TEST_CASE(TestConstruction_NonEmulated)
{
	BOOST_CHECK_NO_THROW(IAQDevice device(device_type, *this, false));
}

// =============================================================================
// QueueCommand
// =============================================================================

BOOST_AUTO_TEST_CASE(TestQueueCommand_DoesNotThrow)
{
	IAQDevice device(device_type, *this, true);
	BOOST_CHECK_NO_THROW(device.QueueCommand(0x19));
}

BOOST_AUTO_TEST_CASE(TestQueueCommand_MultipleCommands)
{
	IAQDevice device(device_type, *this, true);
	BOOST_CHECK_NO_THROW(device.QueueCommand(0x19));
	BOOST_CHECK_NO_THROW(device.QueueCommand(0x02));
	BOOST_CHECK_NO_THROW(device.QueueCommand(0x01));
}

// =============================================================================
// QueueChlorinatorPercentage
// =============================================================================

BOOST_AUTO_TEST_CASE(TestQueueChlorinatorPercentage_DoesNotThrow)
{
	IAQDevice device(device_type, *this, true);
	BOOST_CHECK_NO_THROW(device.QueueChlorinatorPercentage(75));
}

BOOST_AUTO_TEST_CASE(TestQueueChlorinatorPercentage_Zero)
{
	IAQDevice device(device_type, *this, true);
	BOOST_CHECK_NO_THROW(device.QueueChlorinatorPercentage(0));
}

BOOST_AUTO_TEST_CASE(TestQueueChlorinatorPercentage_Hundred)
{
	IAQDevice device(device_type, *this, true);
	BOOST_CHECK_NO_THROW(device.QueueChlorinatorPercentage(100));
}

// =============================================================================
// QueueChlorinatorBoost
// =============================================================================

BOOST_AUTO_TEST_CASE(TestQueueChlorinatorBoost_Enable)
{
	IAQDevice device(device_type, *this, true);
	BOOST_CHECK_NO_THROW(device.QueueChlorinatorBoost(true));
}

BOOST_AUTO_TEST_CASE(TestQueueChlorinatorBoost_Disable)
{
	IAQDevice device(device_type, *this, true);
	BOOST_CHECK_NO_THROW(device.QueueChlorinatorBoost(false));
}

// =============================================================================
// Command sequencing
// =============================================================================

BOOST_AUTO_TEST_CASE(TestQueuePercentage_ThenBoost_DoesNotThrow)
{
	IAQDevice device(device_type, *this, true);
	device.QueueChlorinatorPercentage(50);
	BOOST_CHECK_NO_THROW(device.QueueChlorinatorBoost(true));
}

BOOST_AUTO_TEST_CASE(TestQueueBoost_ThenPercentage_DoesNotThrow)
{
	IAQDevice device(device_type, *this, true);
	device.QueueChlorinatorBoost(true);
	BOOST_CHECK_NO_THROW(device.QueueChlorinatorPercentage(75));
}

// =============================================================================
// Destruction after queuing
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDestruction_AfterQueuing)
{
	{
		IAQDevice device(device_type, *this, true);
		device.QueueCommand(0x19);
		device.QueueChlorinatorPercentage(50);
		device.QueueChlorinatorBoost(false);
	}
	// If we reach here without crash, destruction is clean
	BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()
