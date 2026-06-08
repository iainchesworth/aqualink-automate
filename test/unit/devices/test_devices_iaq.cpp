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

	// Exposes the protected watchdog/update hooks so the start-up state machine can be
	// driven directly. The IAQDevice ctor arms its watchdog on the real clock, so the
	// tests invoke WatchdogTimeoutOccurred() rather than waiting on wall-clock time.
	struct TestIAQDevice : public IAQDevice
	{
		using IAQDevice::IAQDevice;
		void TriggerWatchdogTimeout() { WatchdogTimeoutOccurred(); }
		void SimulateAddressedTraffic() { ProcessControllerUpdates(); }
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

// =============================================================================
// Start-up watchdog states (regression for the live iAqualink2 capture)
// =============================================================================

BOOST_AUTO_TEST_CASE(TestWatchdog_NeverAddressedEmulated_BecomesNotPresentNotFault)
{
	// An emulated IAQ id the master never addresses (e.g. the default 0xa1 on a
	// panel with no iAqualink2 configured there) must settle to NotPresent, NOT
	// fault -- nothing went wrong, the id simply isn't on the bus.
	TestIAQDevice device(device_type, *this, /*is_emulated=*/true);
	BOOST_REQUIRE(!device.IsFaulted());
	BOOST_REQUIRE(!device.IsNotPresent());

	device.TriggerWatchdogTimeout();   // 30s elapsed with no traffic ever addressed

	BOOST_CHECK(device.IsNotPresent());
	BOOST_CHECK(!device.IsFaulted());
}

BOOST_AUTO_TEST_CASE(TestWatchdog_AddressedThenSilent_Faults)
{
	// A device that WAS receiving traffic addressed to its id and then went silent
	// is a genuine fault, distinct from "never present".
	TestIAQDevice device(device_type, *this, /*is_emulated=*/true);
	device.SimulateAddressedTraffic();   // traffic seen -> m_HasReceivedData = true
	device.TriggerWatchdogTimeout();     // ...then it stopped

	BOOST_CHECK(device.IsFaulted());
	BOOST_CHECK(!device.IsNotPresent());
}

BOOST_AUTO_TEST_SUITE_END()
