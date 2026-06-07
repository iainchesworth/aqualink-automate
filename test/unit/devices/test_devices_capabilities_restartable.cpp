#include <chrono>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/capabilities/restartable.h"

using namespace AqualinkAutomate::Devices::Capabilities;

namespace
{
	// Test double: exposes the protected watchdog controls and injects a controllable clock
	// so the deadline logic can be exercised without real-time waits.
	class TestRestartable : public Restartable
	{
	public:
		explicit TestRestartable(std::chrono::seconds timeout)
			: Restartable(timeout, /*delayed_start=*/true)
		{
		}

		void SetNow(std::chrono::steady_clock::time_point now) { m_Now = now; }

		using Restartable::Start;
		using Restartable::Kick;
		using Restartable::Stop;
		using Restartable::CheckWatchdog;
		using Restartable::IsRunning;

		int TimeoutFiredCount() const { return m_FiredCount; }

	protected:
		void WatchdogTimeoutOccurred() override { ++m_FiredCount; }
		std::chrono::steady_clock::time_point Now() const override { return m_Now; }

	private:
		std::chrono::steady_clock::time_point m_Now{};
		int m_FiredCount{ 0 };
	};

	constexpr std::chrono::seconds TIMEOUT{ 30 };
	const std::chrono::steady_clock::time_point T0{ std::chrono::seconds{ 1000 } };
}

BOOST_AUTO_TEST_SUITE(TestSuite_DeviceCapabilities_Restartable)

BOOST_AUTO_TEST_CASE(Watchdog_FiresAfterTimeoutWithoutKick)
{
	TestRestartable wd(TIMEOUT);
	wd.SetNow(T0);
	wd.Start();
	BOOST_CHECK(wd.IsRunning());

	// Just inside the timeout window -> still alive, no fire.
	wd.SetNow(T0 + std::chrono::seconds{ 29 });
	wd.CheckWatchdog();
	BOOST_CHECK(wd.IsRunning());
	BOOST_CHECK_EQUAL(0, wd.TimeoutFiredCount());

	// Past the timeout -> fires once and marks the device not running.
	wd.SetNow(T0 + std::chrono::seconds{ 31 });
	wd.CheckWatchdog();
	BOOST_CHECK(!wd.IsRunning());
	BOOST_CHECK_EQUAL(1, wd.TimeoutFiredCount());
}

BOOST_AUTO_TEST_CASE(Watchdog_KickResetsDeadline)
{
	TestRestartable wd(TIMEOUT);
	wd.SetNow(T0);
	wd.Start();

	// A kick at +20s resets the deadline to +20s.
	wd.SetNow(T0 + std::chrono::seconds{ 20 });
	wd.Kick();

	// +45s is only 25s since the kick -> still alive.
	wd.SetNow(T0 + std::chrono::seconds{ 45 });
	wd.CheckWatchdog();
	BOOST_CHECK(wd.IsRunning());
	BOOST_CHECK_EQUAL(0, wd.TimeoutFiredCount());

	// +51s is 31s since the kick -> fires.
	wd.SetNow(T0 + std::chrono::seconds{ 51 });
	wd.CheckWatchdog();
	BOOST_CHECK(!wd.IsRunning());
	BOOST_CHECK_EQUAL(1, wd.TimeoutFiredCount());
}

BOOST_AUTO_TEST_CASE(Watchdog_StoppedDoesNotFire)
{
	TestRestartable wd(TIMEOUT);
	wd.SetNow(T0);
	wd.Start();
	wd.Stop();
	BOOST_CHECK(!wd.IsRunning());

	wd.SetNow(T0 + std::chrono::seconds{ 1000 });
	wd.CheckWatchdog();
	BOOST_CHECK_EQUAL(0, wd.TimeoutFiredCount());
}

BOOST_AUTO_TEST_SUITE_END()
